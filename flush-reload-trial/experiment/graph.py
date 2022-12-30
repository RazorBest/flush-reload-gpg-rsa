import csv
import matplotlib.pyplot as plt
import numpy as np
import sys
from Levenshtein import distance as lev

CUTOFF = 140
INPUT_FILE = "out.txt"

# Call counts for a specific private key
EXPECTED_SQR_CALL_COUNT = 2044
EXPECTED_REDUCE_CALL_COUNT = 3098
EXPECTED_MUL_CALL_COUNT = 1049
EXPECTED_SECRET = "10111010100101111111110100101001011011000110000001000100010011100100101101000000111000100110110000111100011111111010110100111010000001001011001100001001000110001100101001101111111100000100001101001010110011001101001111000011100100110101001100110101011010011010010101110110011100110001000111101011100111111110110011100111110111010101000000010100100000110100110101001111100001000100000111001101100011110001101001110011110111001000011101010001011111001011111110000000001010111001111101101110001000101000010000111010000111100111001001111011001011001000010111010001000001010011001011100001100110101110001011010011000101000111100001101110101111111110111011110100011101111011101110101001000000000100100000111001110010011010001010010100100011111110100110100011011111000011100010100110001010011001110100011100001000110101100110101000111111110101011110010101001001111000111101010101000111010010000111111101100111000110011101110011001010011110110010000011011100011101110111110111000001001000101000111001110110011000101000100110001011010100111011011100011101110100111100110101000010101001101110101001110100010010010110110110110010000101110110010101110000001000011110110011110110101101111101001111100101111100110111001101011001010001111011111100110111110101110110011011010111000110100100100100010011010101011000001011010110110101110100001111111110011110110001111000001000101101110011111000100000001111000100000111010100101101011000111111101100000110100011011101110101100011111001000000010000101000000110111111101110001111101000100111111100100000010001111000000000001011101000101011101100011010011001000011111001011001000100001010010011110100001100101110001000110010111011100110001010011010111101110111011100100100000001010110100001110010110111000001111100110010010000100111011010000110110010101100111111011011001000000010010000001110010111010100000111010011110101101111100011100010101101010010110111111000101001111011111101100110010010111111100101101100101110010001011011011110101100110010111100010100111100111101011011110111101010000010010010110000101010101"

def get_args():
    global INPUT_FILE

    if len(sys.argv) > 1:
        INPUT_FILE = sys.argv[1]

def plot_running_avg(ax, x, y, window):
    avg_y = []
    for i in range(len(x) - window + 1):
        avg_y.append(np.mean(y[i:i+window]))

    x = x[window-1:]

    ax.plot(x, avg_y)

def plot_running_count_in_range(ax, x, y, lo, hi, window):
    avg_y = []
    for i in range(len(x) - window + 1):
        slide = [val >= lo and val <= hi for val in y[i:i+window]]
        avg_y.append(np.count_nonzero(slide))

    x = x[window-1:]

    ax.plot(x, avg_y)

def get_data():
    data = []

    with open(INPUT_FILE, "r", encoding="utf-8") as file:
        file.readline()
        file.readline()
        csv_file = csv.reader(file, delimiter="\t") 

        for line in csv_file:
            data.append(line)

    return data

def plot():
    data = get_data()
    
    fig, axes = plt.subplots(2, 2)
    ax1 = axes[0, 0]
    ax2 = axes[0, 1]
    ax3 = axes[1, 0]
    ax4 = axes[1, 1]

    slots = [int(row[0]) for row in data]
    window_size = 50;

    times = [int(row[1]) for row in data]
    plot_running_avg(ax1, slots, times, window=window_size)
    #ax.scatter(slots, times, s=1)

    times = [int(row[2]) for row in data]
    plot_running_avg(ax2, slots, times, window=window_size)

    times = [int(row[3]) for row in data]
    plot_running_avg(ax3, slots, times, window=window_size)

    plt.show()

    fig, ax4 = plt.subplots()

    times = [int(row[1]) for row in data] # mpih_sqr_n
    ax4.scatter(slots, times, marker='+', c='r')
    #plot_running_count_in_range(ax4, slots, times, 1, 100, window=window_size)
    times = [int(row[2]) for row in data] # mpihelp_divrem
    ax4.scatter(slots, times, marker='x', c='g')
    #plot_running_count_in_range(ax4, slots, times, 1, 100, window=window_size)
    times = [int(row[3]) for row in data] # mul_n
    ax4.scatter(slots, times, marker='o', s=2, c='b')

    times = [int(row[4]) for row in data] # mul_n (base)
    ax4.scatter(slots, times, marker='3', c='y')

    #ax.locator_params(axis='x', nbins=20)

    #fig.suptitle('Time measurments at each slot')
    plt.show()

def get_call_times(time_axis, in_threshold, out_threshold=None):
    """Takes a list of access time to an address location. Determines
        the function calls corresponding to that address, base on the
        thesholds. Small time means the address was store in cache,
        therefore the function was called.
    """
    if out_threshold is None:
        out_threshold = in_threshold + 1

    assert in_threshold < out_threshold

    calls = []

    in_call = False
    for index, delay in enumerate(time_axis):
        if delay <= in_threshold and not in_call:
            in_call = True
            calls.append((index, in_call))
        elif delay >= out_threshold and in_call:
            in_call = False
            calls.append((index, in_call))

    return calls

def extract_info():
    IN_SQ = 2
    IN_MUL = 3

    data = get_data()

    sq = [int(row[1]) for row in data] # mpih_sqr_n
    dv = [int(row[2]) for row in data] # mpihelp_divrem
    mult = [int(row[3]) for row in data] # mul_n
    mult_b = [int(row[4]) for row in data] # mul_n (base)


    conflict1 = 0
    conflict2 = 0
    conflict3 = 0
    for y1, y2, y3 in zip(sq, mult, dv):
        if y1 == -1 or y2 == -1:
            continue
        if y1 < CUTOFF and y2 < CUTOFF:
            conflict1 += 1
        if y1 < CUTOFF and y3 < CUTOFF:
            conflict2 += 1
        if y2 < CUTOFF and y3 < CUTOFF:
            conflict3 += 1

    print(f"Conflict (sq, mult): {conflict1}")
    print(f"Conflict (sq, div): {conflict2}")
    print(f"Conflict (mult, div): {conflict3}")

    calls = get_call_times(sq, CUTOFF)
    call_count = sum(1 for _, in_call in calls if in_call)
    print(f"Isolated mpih_sqr_n call count: {call_count}")

    calls = get_call_times(dv, CUTOFF)
    call_count = sum(1 for _, in_call in calls if in_call)
    print(f"Isolated mpihelp_divrem call count: {call_count}")

    calls = get_call_times(mult, CUTOFF)
    call_count = sum(1 for _, in_call in calls if in_call)
    print(f"Isolated mul_n call count: {call_count}")

    calls = get_call_times(mult_b, CUTOFF)
    call_count = sum(1 for _, in_call in calls if in_call)
    print(f"Isolated mul_n (from base) call count: {call_count}")

    call_count = 0
    amortise = 0
    in_call = False
    for y1, y2, y3 in zip(sq, mult, dv):
        if y1 == -1:
            continue

        if y1 < CUTOFF and not in_call:
            in_call = True
            call_count += 1
        if y1 >= CUTOFF and y3 < CUTOFF:
            amortise += 1
            if amortise > 0:
                in_call = False
        if y1 < CUTOFF:
            amortise = 0

    print(f"Call count for miph_sqr_n: {call_count}")

    call_count = 0
    amortise = 0
    in_call = False
    mul_acc = 0
    for y1, y2, y3 in zip(mult, mult_b, dv):
        if y1 == -1:
            continue

        if y1 < CUTOFF and not in_call:
            in_call = True
            call_count += 1
        if y1 >= CUTOFF and y3 < CUTOFF:
            amortise += 1
            if amortise > 0:
                in_call = False
        if y1 < CUTOFF:
            amortise = 0

        if y2 < CUTOFF and not in_call:
            mul_acc += 1
            if mul_acc > 1:
                in_call = True
                call_count += 1
                mul_acc = 0
        elif y2 >= CUTOFF and (y3 < CUTOFF or y1 < CUTOFF):
            mul_acc = 0


    print(f"Call count for mul_n: {call_count}")

    calls = []
    amortise1 = 0
    amortise2 = 0
    in_call_1 = False
    in_call_2 = False
    square_last = False
    mul_acc = 0

    call3_wait = 0
    for y1, y2, y3, y4, index in zip(sq, mult, dv, mult_b, range(len(dv))):
        if y1 == -1: # All should be -1 if one is -1
            continue

        if y1 < CUTOFF and not in_call_1:
            in_call_1 = True
        if y1 >= CUTOFF and y3 < CUTOFF:
            amortise1 += 1
            if amortise1 > 0:
                if in_call_1 == True:
                    if square_last:
                        calls.append((0, index))
                    square_last = True
                in_call_1 = False
        if y1 < CUTOFF:
            amortise1 = 0

        if y2 < CUTOFF and not in_call_2 and y3 >= CUTOFF:
            in_call_2 = True
        if y2 >= CUTOFF and y3 < CUTOFF:
            amortise2 += 1
            if amortise2 > 0:
                if in_call_2 == True:
                    calls.append((1, index))
                    square_last = False
                in_call_2 = False
        if y2 < CUTOFF:
            amortise2 = 0

        if y4 < CUTOFF and not in_call_2:
            call3_wait += 1
            if call3_wait > 1:
                in_call_2 = True
                # Why not set call3_wait to 0 ?????????????????????????
        if y4 >= CUTOFF and (y3 < CUTOFF or y2 < CUTOFF):
            call3_wait = 0

        if y2 < CUTOFF and y3 >= CUTOFF:
            mul_acc += 1
        elif y4 < CUTOFF and y3 >= CUTOFF:
            mul_acc += 1
        elif y3 < CUTOFF:
            mul_acc = 0

        if mul_acc > 1:
            if in_call_1:
                in_call_1 = False

    #print(calls[1270:1290])
    extracted_secret = ''.join([str(call[0]) for call in calls])

    lev_dist = lev(extracted_secret, EXPECTED_SECRET)
    print(f"Levensthein distance between extracted and expected secret: {lev_dist}")
    print(f"Accuracy: {1 - lev_dist / len(EXPECTED_SECRET):.3f}")

    print(extracted_secret)


def main():
    get_args()
    #plot()
    extract_info()

if __name__ == "__main__":
    main()
