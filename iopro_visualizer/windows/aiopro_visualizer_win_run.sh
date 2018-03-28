#!/bin/bash
if [ $# -lt 2 ]
then
    echo ====================== Usage ======================
    echo "aiopro_visualizer_run.sh [config_file] [log_dir]"
    exit 0
fi
config_file=$1
log_dir=$2
echo "[config_file]:$config_file"
echo "[log_dir]:$log_dir"
echo ""
echo "python info.py -c $config_file -d $log_dir"
echo ""
python info.py -c $config_file -d $log_dir
echo ""
echo "rm visualizer"
echo ""
rm visualizer
echo ""
echo "gcc -o visualizer visualizer.c"
echo ""
gcc -o visualizer visualizer.c -DDEBUG
echo ""
echo "cd $log_dir"
echo ""
cd $log_dir
echo ""
echo "gtkwave output.gtkw"
echo ""
gtkwave output.gtkw

