import random

LENGTH = 256

def main():
    r = random.Random()
    array = []
    for x in range(LENGTH):
        array.append(x)
    r.shuffle(array)
    print(array)

if __name__ == "__main__":
    main()