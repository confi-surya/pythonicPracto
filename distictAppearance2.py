from collections import Counter, OrderedDict
class OrderedCounting(Counter, OrderedDict):
    pass
wordcounter = OrderedCounting([raw_input() for _ in range(int(raw_input()))])
print len(wordcounter)
print wordcounter
print ' '.join([str(count) for _, count in wordcounter.iteritems()])
