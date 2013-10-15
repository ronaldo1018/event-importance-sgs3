reset

set bmargin at screen 0.2
set xtics nomirror rotate by -45 font "Times-Roman,24"
set ytics 0,20 font "Times-Roman,24" offset 1,0
set yrange [0:100]
set ylabel "Energy Consumption (J)" font "Times-Roman,24"
set key font "Times-Roman,24" spacing 3

set label "H: File Manager" at -0.8,95 font "Times-Roman,24"
set label "M: Video Player" at -0.8,87 font "Times-Roman,24"
set label "L1: ZIP" at -0.8,79 font "Times-Roman,24"
set label "L2: FTP" at -0.8,71 font "Times-Roman,24"

set style data histograms 

# xticlabels(1): use first column as xtic title, linetype -1: color black, '': use the same filename
set term postscript eps enhanced monochrome size 3,3
set output "l1_energy.eps"
plot 'l1_energy.dat' u 2:xtic(1) ti column fs pattern 0 lt -1,\
	'' u 3 ti column fs pattern 5 lt -1,\
	'' u ($0-0.75):($2+4):(sprintf("%.1f%%",($3-$2)/$2*100)) notitle w labels font "Times-Roman,20"
set output
