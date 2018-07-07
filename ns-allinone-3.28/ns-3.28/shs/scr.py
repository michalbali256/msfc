#!/usr/bin/env python2

import sys
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as plticker


upcut = float(sys.argv[1])
detail = int(sys.argv[2])
dropped = []
total = []
data = []
i = -1
next = True
for line in sys.stdin:
    if line == "NEXT\n":
        next = True
        continue
    if next:
        data.append([])
        total.append(0)
        dropped.append(0)
        i = i + 1
        
    
    spl = line.split(" ")
    val = float(spl[0]) + float(spl[1])/2
    count = int(spl[2])
    total[i] += count
    if val != 0 and val > upcut :
        dropped[i] += count
        continue
    
    data[i].extend(np.repeat(val, count))
    #print val
    next = False

print "Total: "
print total
print "Omitted: "
print dropped 

loc = plticker.MultipleLocator(base=0.002) # this locator puts ticks at regular intervals


fig, ax = plt.subplots()
fs = 12
ax.violinplot(data, points=detail, widths=0.3,
                      showmeans=True, showextrema=True)
ax.set_title('Custom violinplot 1', fontsize=fs)
ax.yaxis.set_major_locator(loc)

fig.suptitle("Violin Plotting Examples")
#fig.subplots_adjust(hspace=0.4)
plt.savefig("temp.pdf")
plt.show()