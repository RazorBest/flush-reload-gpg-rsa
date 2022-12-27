from __future__ import print_function

from collections import namedtuple, defaultdict
import csv
import matplotlib.pyplot as plt
import numpy as np
import sys


inputfile = sys.argv[1] if len(sys.argv) == 2 else 'out.txt'

Hit = namedtuple('Hit', ['slot', 'addr', 'time'])

# Before, it was 80
CUTOFF = 3000 #80
square = {'label': 'Square', 'marker': 'x', 'color': '#FF0059', 'addrs': [0]}
red = {'label': 'Reduce', 'marker': '.', 'color': '#2C00E8', 'addrs': [1]}
mult = {'label': 'Multiply', 'marker': '^', 'color': '#00F1FF', 'addrs': [2]}
hit_types = [square, red, mult]

def plot_running_min(ax, window, x, y):
    y_avg = []
    for i in range(len(y) - window + 1):
        y_avg.append(np.min(y[i:i + window]))

    x = x[window - 1:]

    ax.plot(x, y_avg)

def plot_simple():
    rows = []
    with open(inputfile, 'r') as outfile:
        probereader = csv.reader(outfile, delimiter=' ')
        rows = [Hit(slot=int(row[0]), addr=int(row[1]), time=int(row[2]))
                for row in probereader]
        hits = [row for row in rows if row.time < CUTOFF]

    fig, ax = plt.subplots()

    times = [row.time for row in hits if row.addr == 0]
    slots = [row.slot for row in hits if row.addr == 0]
    #times = times[20000:45000]
    #slots = slots[20000:45000]

    plot_running_min(ax, 100, slots, times)

    #fig = plt.figure()
    #axis = fig.add_subplot(111)

    #ax.hist(times, bins=50, range=(0, 180))

#    plt.show()

    times = [row.time for row in hits if row.addr == 1]
    slots = [row.slot for row in hits if row.addr == 1]
    #times = times[20000:50000]
    #slots = slots[20000:50000]

    plot_running_min(ax, 100, slots, times)

    #fig, ax = plt.subplots()
    #fig = plt.figure()
    #axis = fig.add_subplot(111)

    #ax.hist(times, bins=50, range=(0, 180)) 

    times = [row.time for row in hits if row.addr == 2]
    slots = [row.slot for row in hits if row.addr == 2]
    #times = times[20000:50000]
    #slots = slots[20000:50000]

    plot_running_min(ax, 100, slots, times)

    plt.show()


plot_simple()
exit()

with open(inputfile, 'r') as outfile:
    probereader = csv.reader(outfile, delimiter=' ')
    rows = [Hit(slot=int(row[0]), addr=int(row[1]), time=int(row[2]))
            for row in probereader]
    hits = [row for row in rows if row.time < CUTOFF]
    seen = {}
    for hit in hits:
        if hit.slot not in seen:
            seen[hit.slot] = 0.0
        seen[hit.slot] += 1.0
    print(sum(seen.values()) / len(seen.values()))
    print("Slots >= 2 {:d}".format(len([s for s in seen.keys() if seen[s] >= 2])))
    print("Slots == 0 {:d}".format(len([s + 1 for s in seen.keys() if (s + 1) not in seen])))
    print("Slots, tot {:d}".format(len(seen)))

    addr_counts = defaultdict(int)
    for hit in hits:
        addr_counts[hit.addr + 1] += 1
    print("Counts for each address:", addr_counts)

    multiplies = [hit for hit in hits if hit.addr in mult['addrs']]
    slots_to_multiplies = {}
    for multiply in multiplies:
        slots_to_multiplies[multiply.slot] = multiply
    multiply_slots = sorted(slots_to_multiplies.keys())
    dists = {}
    for i in range(0, len(multiply_slots) - 1):
        dist = multiply_slots[i + 1] - multiply_slots[i]
        if dist not in dists:
            dists[dist] = 0
        dists[dist] += 1
    print("Counts for each distance between subsequent multiplies (in slots)", dists)

    for hit_type in hit_types:
        hits_of_type = [hit for hit in hits if hit.addr in hit_type['addrs']]
        slot_to_hits = {}
        for hit in hits_of_type:
            slot_to_hits[hit.slot] = hit
        print("{:s} {:d}".format(hit_type['label'], len(slot_to_hits.keys())))

    fig = plt.figure()
    axis = fig.add_subplot(111)
    plots = []
    # Somewhat of a hack right now, but we assume that there are 5 addresses
    # range instead of xrange for forwards compatibility
    # Also, have to do each address separately because scatter() does not accept
    # a list of markers >_>
    for hit_type in hit_types:
        addr_hits = [hit for hit in hits if hit.addr in hit_type['addrs']]
        slot_to_hits = {}
        for hit in addr_hits:
            slot_to_hits[hit.slot] = hit
        addr_hits = slot_to_hits.values()
        plot = axis.scatter(
            [hit.slot for hit in addr_hits],
            [hit.time for hit in addr_hits],
            c=hit_type['color'],
            marker=hit_type['marker'],
        )
        plots.append(plot)
    # plt.plot([hit.slot for hit in hits], [hit.time for hit in hits])
    plt.legend(tuple(plots),
               tuple([hit_type['label'] for hit_type in hit_types]),
               scatterpoints=1,
               loc='upper right',
               ncol=1)
    plt.show()
