reset

set bmargin at screen 0.2
set xtics nomirror rotate by -45 font "Times-Roman,24"
set ytics 0,60 font "Times-Roman,24"
set yrange [0:240]
set ylabel "Response Time (ms)" font "Times-Roman,24"
set key font "Times-Roman,24" spacing 3

#set label "H: File Manager" at -0.8,190 font "Times-Roman,24"
#set label "L1: Zip" at -0.8,175 font "Times-Roman,24"
#set label "L2: FTP" at -0.8,160 font "Times-Roman,24"

set style data histograms 

# xticlabels(1): use first column as xtic title, linetype -1: color black, '': use the same filename
set term postscript eps enhanced monochrome size 3,3
set output "h_performance.eps"
plot 'h_performance.dat' u 2:xtic(1) ti column fs pattern 0 lt -1,\
	'' u 3 ti column fs pattern 5 lt -1,\
	'' u ($0-0.75):($2+8):(sprintf("%.1f%%",($3-$2)/$2*100)) notitle w labels font "Times-Roman,20"
set output
