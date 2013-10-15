reset

set xtics nomirror font "Times-Roman,24" scale 0 offset 1,0
set ytics font "Times-Roman,24" offset 1,0
set yrange [0:700]
set ylabel "Power Consumption (mW)" font "Times-Roman,24"
set key font "Times-Roman,24" spacing 3 at 0.4,680

set style data histograms 

# xticlabels(1): use first column as xtic title, linetype -1: color black, '': use the same filename
set term postscript eps enhanced monochrome size 3,3
set output "idle_power.eps"
plot 'idle_power.dat' using 2:xticlabels(1) title column fillstyle pattern 0 linetype -1,\
	'' using 3 title column fillstyle pattern 5 linetype -1,\
	'' u ($0-0.75):($3+20):(sprintf("%.1f%%",($3-$2)/$2*100)) notitle w labels font "Times-Roman,20"
#set term png
#set output "idle_power.png"
#replot
set output
