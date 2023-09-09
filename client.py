import socket
import struct

class ProtocolCodes:
    SUCCESS = 0     # Была выполнена одна из команд insert, remove, select, reselect
    PRINT_DATA = 1  # Была выполнена команда print, нужно принять данные
    QUIT = 2        # Была выполнена команда stop или shutdown, нужно прекратить работу
    ERROR = 3       # Возникла ошибка

def getStrFromServer(sock, len):
    data = sock.recv(len, socket.MSG_WAITALL)
    byte_string = struct.unpack(str(len) + "s", data)[0]
    return byte_string.decode()

def getIntFromServer(sock):
    data = sock.recv(struct.calcsize("i"), socket.MSG_WAITALL)
    return struct.unpack("i", data)[0]

def sendStrToServer(sock, query):
    n = len(query)
    packed_data = struct.pack("i", n)
    sock.sendall(packed_data)
    packed_data = struct.pack(str(n) + "s", query.encode())
    sock.sendall(packed_data)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(("localhost", 5555))
print("Welcome!\n")
while True:
    query = input(">> ")
    query = query.strip()
    if query == "":
        continue
    try:
        sendStrToServer(sock, query)
        code = getIntFromServer(sock)
        if code == ProtocolCodes.SUCCESS:
            print("\tYour query was processed successfully!")
            continue
        elif code == ProtocolCodes.PRINT_DATA:
            print("\tThe following information was found for your query:\n")
            num_of_records = getIntFromServer(sock)
            for i in range(num_of_records):
                length = getIntFromServer(sock)
                print('\t' + getStrFromServer(sock, length), end='\n')
        elif code == ProtocolCodes.QUIT:
            print("\nGoodbye!")
            break
        elif code == ProtocolCodes.ERROR:
            length = getIntFromServer(sock)
            print('\t' + getStrFromServer(sock, length), end='\n')
    except Exception as e:
        print("Something went wrong! The server has probably been down.")
        break
sock.close()
