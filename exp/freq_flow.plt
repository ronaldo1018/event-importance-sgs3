reset

set xtics nomirror 0,10,80 font "Times-Roman,24"
set ytics 200,200,1400 font "Times-Roman,24" offset 1,0
set yrange [0:1600]
set ylabel "CPU Frequency (MHz)" font "Times-Roman,24"
set key font "Times-Roman,24" spacing 3

set style data points 

# xticlabels(1): use first column as xtic title, linetype -1: color black, '': use the same filename
set term postscript eps enhanced monochrome size 5,2
set output "freq_orig_flow.eps"
plot 'flow.dat' using 1:4 title "NATIVE" pointtype 1 pointsize 1.5
set output "freq_UOSG_flow.eps"
plot 'flow.dat' using 1:5 title "UCSG" pointtype 2 pointsize 1.5
set output 

