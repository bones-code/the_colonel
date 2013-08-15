# Defines the functions needed for the translation of evlog.txt.
from keymap import keys, cap_keys, cap_shift_keys, shift_keys


# Accepts the loop iterator (num) and the key symbol 
# to be counted (key_sym). Counts the number of 
# occurences of the specified key_sym.
def counter(ev, num, key_sym):
    if ev[num] != key_sym:
        return 0
    return 2 + counter(ev, num-1, key_sym)


# Accepts the count (c) and loop iterator (num), 
# then deletes that number of items beginning with
# the list index determined by the iterator.
def del_count(ev, num, c):
    if 0 == c:
        return num
    del ev[num]
    del_count(ev, num-1, c-1)


def which_keymap(ev, num, km):
    # For cases when a letter is depressed.
    if 2 == ev[num][1]:
        ev.insert(num+1, km[ev[num][0]])
        ev[num] = km[ev[num][0]]
    else:
        ev[num] = km[ev[num][0]]


def translate(f):
    evlst = []
    caps = False
    shift = False
    evlog = f.split('-')

    # Splits the received keylog into a list.
    for ev in evlog:
        if len(ev) > 5:
            evlst.append(ev)
            continue
        ev = ev.split(',')
        try:
            ev[0] = int(ev[0])
            ev[1] = int(ev[1])
            evlst.append(ev)
        except:
            continue
    
    # Translates the keylog list (evlst).
    for n in reversed(xrange(len(evlst))):
        # Skips translating the date.
        if len(evlst[n]) > 5:
            continue
        if '[SHIFT]' == keys[evlst[n][0]] and 0 == evlst[n][1]:
            shift = True
            del evlst[n]
            continue
        elif '[SHIFT]' == keys[evlst[n][0]] and 1 == evlst[n][1]:
            shift = False
            del evlst[n]
            continue
        
        if 0 == evlst[n][1]:
            del evlst[n]
            continue
        
        if '[CAPSLOCK]' == keys[evlst[n][0]] and False == caps:
            caps = True
            del evlst[n]
            continue
        elif '[CAPSLOCK]' == keys[evlst[n][0]] and True == caps:
            caps = False
            del evlst[n]
            continue

        # Determines which keymap to use for translation.
        if shift and caps:
            which_keymap(evlst, n, cap_shift_keys)
        elif shift:
            which_keymap(evlst, n, shift_keys)
        elif caps:
            which_keymap(evlst, n, cap_keys)
        else:
            which_keymap(evlst, n, keys)

        if '[ENTER]' == evlst[n]:
            evlst[n] = ' [ENTER]\n'

    i = len(evlst)-1
    while i >= 0:
        if '[BACKSPACE]' == evlst[i]:
            c = counter(evlst, i, '[BACKSPACE]')
            del_count(evlst, i, c)
            i -= c
        i -= 1

    return "".join(evlst)
