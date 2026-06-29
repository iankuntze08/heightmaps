import random

LENGTH = 256

def main():
    r = random.Random()
    array = list(range(LENGTH))
    r.shuffle(array)
    print(array)

if __name__ == "__main__":
    main()