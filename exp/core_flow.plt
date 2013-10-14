reset

set lmargin at screen 0.11
set xtics nomirror 0,10,80 font "Times-Roman,24"
set ytics 1,1,4 font "Times-Roman,24" offset 1,0
set ylabel offset -3.2,0
set yrange [0:4]
set ylabel "Number of Active Cores" font "Times-Roman,24"
set key font "Times-Roman,24" spacing 3

set style data points 

# xticlabels(1): use first column as xtic title, linetype -1: color black, '': use the same filename
set term postscript eps enhanced monochrome size 5,2
set output "core_orig_flow.eps"
plot 'flow.dat' using 1:2 title "NATIVE" pointtype 1 pointsize 1.5
set output "core_UOSG_flow.eps"
plot 'flow.dat' using 1:3 title "UCSG" pointtype 2 pointsize 1.5
set output 

