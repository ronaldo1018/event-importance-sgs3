reset

set xtics nomirror font "Times-Roman,22"
set ytics font "Times-Roman,22"
set xrange [100:1500]
set yrange [400:2600]
set xlabel "Frequency (MHz)" font "Times-Roman,22"
set ylabel "Power (mW)" font "Times-Roman,22"
set key invert at 970,2350 font "Times-Roman,24" spacing 3

set style data linespoints

set label "Threshold of turning on cores -----" font "Times-Roman,24" at 200,2400
# threshold 1 core to 2 cores (400)
set arrow from 400,662.13 to 200,654.59 nohead lt 2
# threshold 2 cores to 3 cores (800)
set arrow from 400,724.94 to 266.67,718.37 nohead lt 2
# threshold 3 cores to 4 cores (1700)
set arrow from 566.67,894.21 to 425,893.24 nohead lt 2

set term postscript eps enhanced
set output "powermodel.eps"
plot 'powermodel.dat' using 1:2 lt 1 pt 1 title column,\
	'' using 1:3 lt 1 pt 2 title column,\
	'' using 1:4 lt 1 pt 4 title column,\
	'' using 1:5 lt 1 pt 5 title column
	
#set term png
#set output "powermodel.png"
#replot
set output
