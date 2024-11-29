import socket
import ipaddress
import paramiko 
from getmac import get_mac_address
import matplotlib.pyplot as plt
import numpy as np
import time
import sys
import time
import threading
import pyqtgraph as pg
from PyQt5 import QtCore, QtWidgets
from threading import Thread
import random

class MainWindow(QtWidgets.QMainWindow):
    """
    This class represents a plot for visual presentation.

    
    Attributes
    ----------
    thread (Thread) : network stream to receive data from the device
    res (str) : scan results
    

    Methods
    -------
    __init__(thread)
        Initializing the MainWindow class

    update_plot()
        Updating plotting results
    """
    def __init__(self, thread):
        super().__init__()
        self.thread = thread
        self.res = self.thread.results
        self.plot_graph = pg.PlotWidget()
        self.setCentralWidget(self.plot_graph)
        self.plot_graph.setBackground("w")
        pen = pg.mkPen(color=(255, 0, 0))
        styles = {"color": "blue", "font-size": "18px"}
        self.plot_graph.showGrid(x=True, y=True)
        self.plot_graph.setYRange(-4500, 4500)
        self.time = np.arange(0, 65532, 8)
        self.adc_voltage = self.res
        self.line = self.plot_graph.plot(
            self.time,
            self.adc_voltage,
            name="LUS",
            pen=pen,
        )
        self.timer = QtCore.QTimer()
        self.timer.setInterval(10)
        self.timer.timeout.connect(self.update_plot)
        self.timer.start()

    def update_plot(self):
        self.res = self.thread.results
        self.line.setData(self.time, self.res)

class LUS:
    """
    This class represents a LUS device.

    
    Attributes
    ----------
    host (str) : device IP address
    status (bool) : status of device searching success on the network
    mac (list) : list of all devices' mac addresses
    hostname (str) : code word for searching for devices on the local network
    user (str) : name of user
    password (str) : password of user
    network (IPv4Network | IPv6Network) : list of all IP adresses
    port (int) : number of port
    BUFFER_SIZE (int) : buffer size
    TCP_PORT (int) : port of server
    names (list) : list of names of devices
    time (NDArray) : scan timeline
    results (list) : scan results
    thread (Thread) : network stream to receive data from the device
    local (bool) : device connection status (directly / via router)
    

    Methods
    -------
    __init__()
        Initializing the LUS class

    searching_in_local_network()
        Searching for a device on the local network when directly connected to a computer by names 
        contained in "self.names"

    searching_by_mac()
        Searching for a device on the local network when connecting to a computer via a router 
        using the MAC addresses contained in “self.mac”

    searching_by_hostname()
        Searching for a device on the local network when connecting to a computer via a router 
        using the code word contained in “self.hostname”

    host_searching()
        Searching for a device on the network, connecting to the found address, creating a server

    command_sending(command)
        Sending a command to a device

    socket_checker(msg, flag)
        Checking the correctness of the input packet from the client side

    scanning(n)
        Starting scanning with fixed n

    file_getting()
        Receiving data from an input packet

    results_plotting()
        Plotting data obtained from an input packet

    stop_scanning()
        Stopping scanning
    """
    def __init__(self):
        self.host = None
        self.status = False
        self.mac = ["00:26:32:f0:3f:4f", "00:26:32:f0:4c:ff","00:26:32:f0:40:0e"]
        self.hostname = "rp-"
        self.user = "root"
        self.password = "root"
        self.network = ipaddress.ip_network('192.168.1.0/24')
        self.port = 22
        self.BUFFER_SIZE = 8192
        self.TCP_PORT = 5000
        self.names = ["rp-f03f4f", "rp-f0400e", "rp-f04cff"]
        self.time = np.arange(0, 65532, 8)
        self.results = [-1] * 8192
        self.thread = 0
        self.local = False
        self.key_adr = "Здесь_путь_к_private_key"

    
    def searching_in_local_network(self):
        
        socket_success = False
        for name in self.names:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.07)
                
                result = sock.connect_ex((name + ".local", 22))
                if result == 0:
                    socket_true_ip = name + ".local"
                    socket_success = True

                sock.close()

                if socket_success:
                    self.local = True
                    self.host = socket_true_ip
                    return 
                
            except:
                pass
        self.host = None
        return

   
    def searching_by_mac(self):
        
        socket_success = False
        for ip in self.network:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.07)
                
                result = sock.connect_ex((str(ip), 22))
                if result == 0:
                    mac_by_ip = get_mac_address(ip = str(ip))

                    for mac in self.mac:
                        if str(mac_by_ip) == mac:
                            socket_true_ip = str(ip)
                            socket_success = True

                sock.close()

                if socket_success:
                    self.local = False
                    self.host = socket_true_ip
                    return 
                
            except:
                pass
        self.host = None
        return

   
    def searching_by_hostname(self):

        socket_success = False
        for ip in self.network:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(0.07)
                
                result = sock.connect_ex((str(ip), 22))

                if result == 0:
                    socket_name = socket.getnameinfo((str(ip), 0), socket.NI_NAMEREQD)[0]
                    if str(socket_name)[:3] == self.hostname:
                        socket_true_ip = str(ip)
                        socket_success = True

                sock.close()

                if socket_success:
                    self.local = False
                    self.host = socket_true_ip
                    return 
                
            except:
                pass
        self.host = None
        return

    #Тестовая функция для подключения к лусу со статическим IP 
    def searching_static_lus(self):

        self.local = False
        self.host = "192.168.1.115"
        return 
    
    def host_searching(self):

        self.searching_static_lus()
        if self.host == None:
            return
        self.status = True

        #Создаем папку com, создаем файл iplist.txt, проверяем ip, с которого подключились к ЛУСу по SSH, записыаем данный ip в файл
        self.command_sending('mkdir /root/gui/com')
        self.command_sending('touch /root/gui/com/iplist.txt')
        self.command_sending('> /root/gui/com/iplist.txt')
        #if self.local == False:
        #    self.command_sending('echo "' + str(socket.gethostbyname(socket.gethostname())) + '" >> /root/gui/com/iplist.txt')
        #else:
        #    self.command_sending('echo "' + str(socket.gethostbyname_ex(socket.gethostname())[-1][1]) + '" >> /root/gui/com/iplist.txt')
        self.command_sending('echo "${SSH_CLIENT%% *}" | tee /root/gui/com/iplist.txt')
        self.command_sending("tr -d '\n' < /root/gui/com/iplist.txt | tee /root/gui/com/iplist.txt")

        #Создаем SSH подключение к ЛУСу (на данный момент подключение осуществляется по терминалу)
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(hostname = self.host, username = self.user, key_filename=self.key_adr, port =self.port)#password = self.password, port = self.port)

        #Обход проблем с множеством сетевых адаптеров (берем файл с ip, сохраняем в переменную)
        sftp_client = client.open_sftp()
        with sftp_client.open('/root/gui/com/iplist.txt') as f:
            IPAddr = (f.read()).decode("utf-8")

        #Создаем папку com, создаем файл command.txt, прописываем букву "А" в файл (команда для вывода таблицы на экран ЛУСа)
        self.command_sending('mkdir /root/gui/com')
        self.command_sending('touch /root/gui/com/command.txt')
        self.command_sending('> /root/gui/com/command.txt')
        self.command_sending('echo "A" >> /root/gui/com/command.txt')

        #hostname=socket.gethostname()
        #if self.local == False:
        #    IPAddr = socket.gethostbyname(hostname)
        #else:
        #    IPAddr = socket.gethostbyname_ex(socket.gethostname())[-1][1]

        #Инициализируем сокет, бинд на ip ПК с портом 5000, ожидаем подключение одного устройства
        self.sock = socket.socket()
        self.sock.bind((str(IPAddr), 5000))
        self.sock.listen(1)
        self.conn, addr = self.sock.accept()
        print ('Подключение:', addr)
        return

    
    def command_sending(self, command):
        '''
        Parameters:
            command (str): Number of averaging
        '''
        #Если ЛУС найден в сети, инициализируем SSH соединение и отправляем команду через терминал 
        if self.status == True:
            client = paramiko.SSHClient()
            client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            client.connect(hostname = self.host, username = self.user, key_filename=self.key_adr, port =self.port)
            stdin, stdout, stderr = client.exec_command(command)
            data = stdout.read() + stderr.read()
            client.close()
        else:
            print("Устройство не найдено!")
            return
    

    def socket_checker(self, msg, flag):
        '''
        Parameters:
            msg (bytearray) : Number of averaging
            flag (bool) :
        '''
        #buff - сырые данные от ЛУСа, ms - декодированный массив без концевых знаков '\x00n' пакета 
        c = 'n'
        buff = msg
        s = buff.decode("utf-8")
        ms = s.replace('n', '').replace('\x00', '').split(", ")
        position = [pos for pos, char in enumerate(s) if char == c]
        #Если появление концевого знака в сырых данных одно - обрезаем до знака, остальное остается в буфере
        if len(position) == 1:
            ms = s[:position[0]].replace('n', '').replace('\x00', '').split(", ")
            flag = True 
        #Если появлений концевого знака в сырых данных два - обрезаем от знака до знака, остальное остается в буфере
        elif len(position) == 2:
            ms = s[position[0]:position[1]].replace('n', '').replace('\x00', '').split(", ")
            flag = True
        #Если появлений концевого знака в сырых данных нет - добираем данные с ЛУСа
        else:
            buff += self.conn.recv(3000)

        return flag, buff, ms
    

    def scanning(self, n):
        '''
        Parameters:
            n (int): Number of averaging
        '''
        #Массив с возможным количеством усреднений
        default_n = [1, 20, 50, 100, 200, 500, 1000]
        #Если число усреднений от пользователя приемлимо, создаем текстовый файл с командой S + число усреднений и инициализируем поток
        if n in default_n:
            self.command_sending('mkdir /root/gui/com')
            self.command_sending('touch /root/gui/com/command.txt')
            self.command_sending('> /root/gui/com/command.txt')
            self.command_sending('echo "S' + str(n) + '" >> /root/gui/com/command.txt')
            self.thread = Thread(target = self.file_getting)
            self.thread.start()

        #Иначе выводим ошибку
        else:
            self.command_sending('> /root/gui/com/command.txt')
            print("Недопустимое число усреднений!")
            return
        

    def file_getting(self):

        global results
        self.conn.settimeout(2)
        flag = False
        #Инициализируем поток, назначаем атрибут "do_run" (пока поток, отвечающий за сканирование - True, продолжаем выполнение кода)
        t = threading.currentThread()
        while getattr(t, "do_run", True):
            #Пробуем получить с ЛУСа пакет размером 3000 (оптимальное значение относительно экспериментов), в случае успеха декодируем и убираем концевые знаки
            try:
                msg = self.conn.recv(3000)
                ms = msg.decode("utf-8").replace('n', '').replace('\x00', '').split(", ")
            #Если таймайут - пробуем получить данные снова
            except socket.timeout as e:
                err = e.args[0]
                if err == 'timed out':
                    time.sleep(1)
                    print("Устройство перестало отвечать, попробуйте позже.")
                    continue
                else:
                    print(e)
                    sys.exit(1)
            #При любой другой ошибке просто выводим ошибку
            except socket.error as e:
                print(e)
                sys.exit(1)
            #При получении массива с нулевой длиной (т.е. устройство было отключено) выводим сообщение об отключении ЛУСа
            else:
                if len(msg) == 0:
                    print("Устройство было отключено!")
                    sys.exit(0)
                else:
                    prev_len = 0
                    h = 0

                    #Если массив данных меньше 1891 (1892 = 2^13 - столько значений приходит в ЛУС с ПЛИСа) или флаг завершенности пакета False - вызываем ф-ю socket.checker
                    while len(ms) < 8191 or flag == False:
                        if prev_len == len(ms):
                            h+=1
                            if h == 3:
                                break
                        prev_len = len(ms) 
                         
                        flag, msg, ms = self.socket_checker(msg, flag)

                    flag = False
                    
                    ms = [x for x in ms if x != '']
                    buff_len = 8192 - len(ms)
                    #Если длина сырых данных меньше положенного - добираем в нчало массива несколько случайных значений (там все равно шум)
                    if buff_len >= 0:
                        l = [random.randint(-15, 15) for i in range(buff_len)]
                        ms = (ms[::-1] + l)[::-1]
                    #Если длина сырых данных больше положенного - удаляем лишние элементы из "чистового" массива
                    elif buff_len < 0:
                        while len(ms) > 8192:
                            del ms[-1]
                            
                    #Интуем результаты (там все равно инт, только с нулевой дробью), атрибутом потока results назначаем нашу переменную res
                    res = [int(i) for i in ms]
                    self.results = res
                    setattr(t, "results", self.results)

        self.conn.close()


    def results_plotting(self):

        #Создаем граф.окно Qt 
        app = QtWidgets.QApplication([])
        main = MainWindow(self.thread)
        main.show()
        app.exec()  


    def stop_scanning(self):

        #Атрибут потока do_run ставим False
        self.thread.do_run = False
        print("Устройство завершило сканирование.")
        #Создаем файл command.txt с командой E (остановка сканирования)
        self.command_sending('mkdir /root/gui/com')
        self.command_sending('touch /root/gui/com/command.txt')
        self.command_sending('> /root/gui/com/command.txt')
        self.command_sending('echo "E" >> /root/gui/com/command.txt')
        return
