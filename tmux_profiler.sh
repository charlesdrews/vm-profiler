#!/bin/sh
#~/Dropbox/Charlie/NYU/2014_Spring/VMs/project/phase3/tmux_profiler.sh
#
# launch new tmux session or attach if already exists
tmux has-session -t profiler
if [ $? != 0 ]
then
	tmux new-session -s profiler -n main -d # new session (-d = detach)
	tmux split-window -h -l 85 -t profiler:0.0 # split 0:0 to make 0:1 for Vim
	tmux split-window -v -l 10 -t profiler:0.0 # split 0:0 to make 0:2 for log

	# set up pane 0:2 with git live log
	tmux select-pane -t profiler:0.2
	tmux send-keys -t profiler:0.2 'export COLUMNS' C-m
	tmux send-keys -t profiler:0.2 \
		'cd ~/Dropbox/Charlie/NYU/2014_Spring/VMs/project/phase3/' \
		C-m
	tmux send-keys -t profiler:0.2 \
		'~/Dropbox/Charlie/Scripts/git_live_log.sh' C-m

	# set up pane 0:0 with command line
	tmux select-pane -t profiler:0.0
	tmux send-keys -t profiler:0.0 \
		'cd ~/Dropbox/Charlie/NYU/2014_Spring/VMs/project/phase3/' \
		C-m
	tmux send-keys -t profiler:0.0 'ls -ACF --color=auto' C-m

	# set up pane 0:1 with 
	tmux select-pane -t profiler:0.1
	tmux send-keys -t profiler:0.1 'vim profiler.c' C-m
fi
tmux attach -t profiler # re-attach
