#!/usr/bin/env python2

import sys
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as plticker
import scipy.stats as ss

plotname = sys.stdin.readline()

upcut = float(sys.argv[1])
detail = int(sys.argv[2])

dropped = []
total = []
visData = []
allData = []
names = []
name = ""
i = -1
next = True
for line in sys.stdin:
    if line[0:4] == "NEXT":
        next = True
        name = line[5:-1]
        continue
    if next:
        visData.append([])
        allData.append([])
        total.append(0)
        dropped.append(0)
        names.append(name)
        i = i + 1
        
    
    spl = line.split(" ")
    val = float(spl[0]) + float(spl[1])/2
    count = int(spl[2])
    total[i] += count
    
    allData[i].extend(np.repeat(val, count))
    if val != 0 and val > upcut :
        dropped[i] += count
        continue
    
    visData[i].extend(np.repeat(val, count))
    
    next = False

names.append(name)

print "Total: "
print total
print "Omitted: "
print dropped 

#loc = plticker.MultipleLocator(base=0.002) # this locator puts ticks at regular intervals
hmeans = []
inds = []
i = 1
print "Harmonic Means: "
for one in allData:
    h = ss.hmean(one)
    if h <= upcut :
        hmeans.append(h)
        inds.append(i)
    i += 1
    print h

fig, ax = plt.subplots(figsize = (6,8))
fs = 12
parts = ax.violinplot(visData, points=detail, widths=0.4,
                      showmeans=False, showextrema=False)
#ax.set_title('Custom violinplot 1', fontsize=fs)

#ax.yaxis.set_major_locator(loc)

for pc in parts['bodies']:
    pc.set_facecolor('#D43F3A')
    pc.set_edgecolor('black')
    pc.set_alpha(1)

names = names[1:]


ax.scatter(inds, hmeans, marker='o', color='blue', s=15, zorder=3)

ax.get_xaxis().set_tick_params(direction='out')
ax.xaxis.set_ticks_position('bottom')
ax.set_xticks(np.arange(1, len(names) + 1))
ax.set_xticklabels(names)
ax.set_xlim(0.25, len(names) + 0.75)
#ax.set_xlabel('Sample name')


fig.suptitle(plotname)
#fig.subplots_adjust(hspace=0.4)
plt.savefig(plotname[0:-1] + ".pdf")
plt.show()

