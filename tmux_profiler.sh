#!/bin/sh
#~/Dropbox/Charlie/NYU/2014_Spring/VMs/project/phase3/tmux_profiler.sh
#
# launch new tmux session or attach if already exists
tmux has-session -t profiler
if [ $? != 0 ]
then
	tmux new-session -s profiler -n main -d
	tmux send-keys -t profiler \
		'cd ~/Dropbox/Charlie/NYU/2014_Spring/VMs/project/phase3/' \
		C-m
	tmux split-window -h -l 85 -t profiler:0.0
	tmux split-window -v -l 10 -t profiler:0.0
	tmux send-keys -t profiler:0.0 'ls -ACF --color=auto' C-m
	tmux send-keys -t profiler:0.1 'vim profiler.c' C-m
	tmux send-keys -t profiler:0.2 \
		'~/Dropbox/Charlie/Scripts/git_live_log.sh' C-m
	tmux select-pane -t profiler:0.0
	tmux select-pane -t profiler:0.1
fi
tmux attach -t profiler
