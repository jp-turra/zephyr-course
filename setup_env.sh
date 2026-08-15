#!/usr/bin/env bash

linkserver_path="/usr/local/LinkServer/"
west_completion_path="$HOME/.west-completion.bash"

export PATH="$PATH:$linkserver_path"

if [ -f "$west_completion_path" ]; then
    source "$west_completion_path"
fi