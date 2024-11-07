# script for running on GCP (or any other) VM

#!/bin/bash

SESSION="orderbookapp"

tmux new-session -d -s $SESSION

# should have venv in same dir as this script (root dir)
tmux send-keys "cd fapi && venv/bin/python -m fastapi run fapi.py" C-m

tmux split-window -v
tmux send-keys "aeronmd" C-m

tmux split-window -h
tmux send-keys "cd orderbook && ./build/server" C-m

tmux split-window -v
tmux send-keys "cd orderbook && ./build/simulationClient" C-m

tmux select-layout tiled

tmux attach -t $SESSION