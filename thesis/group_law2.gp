unset border
unset key
unset xzeroaxis
unset yzeroaxis
unset xtics
unset ytics

e(x)=sqrt(x**3-2*x+1)
x1=-(1.0+sqrt(5.0))/2.0
x2=(sqrt(5.0)-1.0)/2.0
x3=1.0
d21=x2-x1

g6(t)=-e(x3+t-(x2+3*d21+2))
g5(t)=(x2+3*d21<=t && t<=x2+3*d21+2) ? e(3-(t-(x2+3*d21))) : g6(t)
g4(t)=(x2+2*d21<=t && t<=x2+3*d21) ? 1.0/0.0 : g5(t)
g3(t)=(x2+  d21<=t && t<=x2+2*d21) ? e(x1+t-(x2+d21)) : g4(t)
g2(t)=(x2      <=t && t<=x2+  d21) ? -e(2*x2-t) : g3(t)
g1(t)=(x1      <=t && t<=x2      ) ? e(t) : g2(t)
g(t)=g1(t)

f6(t)=x3+t-(x2+3*d21+2)
f5(t)=(x2+3*d21<=t && t<=x2+3*d21+2) ? 3-(t-(x2+3*d21)) : f6(t)
f4(t)=(x2+2*d21<=t && t<=x2+3*d21) ? 1.0/0.0 : f5(t)
f3(t)=(x2+  d21<=t && t<=x2+2*d21) ? (x1+t-(x2+d21)) : f4(t)
f2(t)=(x2      <=t && t<=x2+d21  ) ? (2*x2-t) : f3(t)
f1(t)=(x1      <=t && t<=x2      ) ? t : f2(t)
f(t)=f1(t)

de(x,Y)=(Y>0?1.0:-1.0)*(e(x+0.0001)-e(x))/0.0001

Px=-0.463
Py=-e(Px)

tangent(x,X,Y) = de(X,Y)*(x-X) + Y

Rx=1.183
Ry=-e(Rx)


set style arrow 1 nohead linetype 2 linewidth 1
set style arrow 2 head filled size graph 0.02,20 linetype 2 linecolor 0 linewidth 1

set arrow from -2,tangent(-2,Px,Py) to 2.5,tangent(2.5,Px,Py) arrowstyle 1
set arrow from Rx,Ry to Rx,-Ry-0.1 arrowstyle 2

lv=0.4
lh=0.2

set label '$P$' at Px-lh,Py+lv
set label '$R$' at Rx-lh,-Ry+lv

set xrange [-2:3]
set yrange [-2.5:2.5]
set samples 1000

set term tikz plotsize 9,6
set output 'group_law2.tex'

set parametric
set trange [x1:15]
plot f(t),g(t) lc 1, sprintf("< echo %f %f; echo %f %f; echo %f %f",Px,Py,Rx,Ry,Rx,-Ry) w p pt 7 ps 1
