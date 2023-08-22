from simpletcp.tcpserver import TCPServer
import math

def get_prime_factors(number):
    prime_factors = []
    while number % 2 == 0:
        prime_factors.append(2)
        number = number / 2
    for i in range(3, int(math.sqrt(number)) + 1, 2):
        while number % i == 0:
            prime_factors.append(int(i))
            number = number / i
    if number > 2:
        prime_factors.append(int(number))
    return prime_factors

def echo(ip, queue, data):
    l = get_prime_factors(int(data))
    queue.put(bytes(str(l), 'utf-8'))

server = TCPServer("0.0.0.0", 5000, echo)
server.run()
