""" id name year cgpa """
l=[(4,"Surya",2014,6.5),
   (1,"Atul",2015,7),
   (3,"Jatin",2017,7.2),
   (5,"Tarun",2016,7.0),
   (2,"Kapil",2018,7.5)]

print l.sort()
#[(1, 'Atul', 2015, 7), (2, 'Kapil', 2018, 7.5), (3, 'Jatin', 2017, 7.2), (4, 'Surya', 2014, 6.5), (5, 'Tarun', 2016, 7.0)]
print sorted(l)
print sorted(l, key=lambda x: x[3])
print sorted(l, key=lambda x: x[2])
print sorted(l, key=lambda x: x[1])
