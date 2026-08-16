#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PSK  CONFIG_WIFI_PSK
#define HTTP_PORT 80
#define RECV_BUF_SIZE 512

LOG_MODULE_REGISTER(wifi_station, LOG_LEVEL_INF);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
bool led_on = false;

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback dhcp_cb;
static K_SEM_DEFINE(got_ip_sem, 0, 1);
static K_SEM_DEFINE(led_sem, 0, 1);

static const char http_response[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n\r\n"
    "<!DOCTYPE html><html><body>"
    "<h1>Controle do LED</h1>"
    "<form action=\"/led\" method=\"GET\">"
    "<button type=\"submit\">Toggle LED</button>"
    "</form>"
    "</body></html>";

static int configure_led(void)
{
    if (!gpio_is_ready_dt(&led)) return -ENODEV;

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) return ret;

    return 0;
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
                                     uint64_t mgmt_event,
                                     struct net_if *iface)
{
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: 
    {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status) {
            LOG_ERR("WiFi falhou ao conectar: %d", status->status);
        } else {
            LOG_INF("WiFi conectado");
        }
        break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        LOG_INF("WiFi desconectado");
        break;
    default:
        break;
    }
}

static int wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_connect_req_params params = {
        .ssid = (uint8_t*) WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = (uint8_t*) WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL
    };

    net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
                                  NET_EVENT_WIFI_CONNECT_RESULT |
                                  NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    return net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}

static void dhcp_handler(struct net_mgmt_event_callback *cb,
                          uint64_t mgmt_event,
                          struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        char buf[NET_IPV4_ADDR_LEN];
        for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
            struct net_if_addr *if_addr = &iface->config.ip.ipv4->unicast[i].ipv4;
            if (if_addr->addr_type != NET_ADDR_DHCP) continue;

            net_addr_ntop(NET_AF_INET, &if_addr->address.in_addr, buf, sizeof(buf));
            LOG_INF("IP obtido: %s", buf);
        }
        k_sem_give(&got_ip_sem);
    }
}

static void dhcp_configure()
{
    net_mgmt_init_event_callback(&dhcp_cb, dhcp_handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&dhcp_cb);
}

static void http_server_thread(void)
{
    k_sem_take(&got_ip_sem, K_FOREVER);   // espera IP antes de abrir socket
    LOG_INF("IP obtido, subindo servidor HTTP");
    
    int server_fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
    if (server_fd < 0) {
        LOG_ERR("Falha ao criar socket: %d", errno);
        return;
    }

    struct net_sockaddr_in addr = {
        .sin_family = NET_AF_INET,
        .sin_port = net_htons(HTTP_PORT),
        .sin_addr.s_addr = NET_INADDR_ANY
    };

    if (zsock_bind(server_fd, (struct net_sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERR("Falha no bind: %d", errno);
        zsock_close(server_fd);
        return;
    }

    if (zsock_listen(server_fd, 1) < 0) {
        LOG_ERR("Falha no listen: %d", errno);
        zsock_close(server_fd);
        return;
    }

    LOG_INF("Servidor HTTP escutando na porta %d", HTTP_PORT);

    while (1) {
        struct net_sockaddr_in client_addr;
        net_socklen_t client_len = sizeof(client_addr);
        int client_fd = zsock_accept(server_fd, (struct net_sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            continue;
        }

        char buf[RECV_BUF_SIZE];
        int len = zsock_recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';

            LOG_INF("Buffer: %s", buf);

            if (strstr(buf, "GET /led") != NULL) {
                LOG_INF("Toggle LED Request");
                led_on = !led_on;
                k_sem_give(&led_sem);
            }

            zsock_send(client_fd, http_response, sizeof(http_response) - 1, 0);
        }

        zsock_close(client_fd);
    }
}

K_THREAD_DEFINE(http_server, 4096, http_server_thread, NULL, NULL, NULL, 5, 0, 2000);

int main(void)
{
    if (configure_led() < 0) {
        LOG_ERR("Falha ao configurar LED");
        return -1;
    }

    if (wifi_connect() < 0) {
        LOG_ERR("Falha ao iniciar conexão WiFi");
        return -1;
    }

    dhcp_configure();
    LOG_INF("Aguardando endereço IP");

    while (1)
    {
        k_sem_take(&led_sem, K_FOREVER);
        LOG_INF("Setting LED %s", led_on ? "ON" : "OFF");
        gpio_pin_set_dt(&led, led_on ? 1 : 0);
    }

    return 0;
}