# script for running on GCP (or any other) VM

#!/bin/bash

SESSION="orderbookapp"

rm orderbook.db
cd orderbook
make build
cd ..

tmux kill-session -t $SESSION
tmux new-session -d -s $SESSION

# should have venv in fapi/ dir
tmux send-keys "cd fapi && venv/bin/python -m fastapi run fapi.py" C-m

tmux split-window -v
tmux send-keys "cd orderbook && ./build/server" C-m

tmux split-window -v
tmux send-keys "cd orderbook && ./build/simulationClient" C-m

tmux select-layout tiled

# tmux attach -t $SESSION