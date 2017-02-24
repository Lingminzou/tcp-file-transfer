using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using System.Timers;

namespace tf_server
{
    class Program
    {
        static string recvCmdForUdp()
        {
            const int listenPort = 8899;
            string cmd = null;

            UdpClient listener = new UdpClient(listenPort);
            IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);

            try
            {
                Console.WriteLine("Waiting for broadcast........");

                byte[] bytes = listener.Receive(ref groupEP);

                cmd = Encoding.ASCII.GetString(bytes, 0, bytes.Length);

                Console.WriteLine("Received broadcast from {0}", groupEP.ToString());

                //Console.WriteLine("Received broadcast from {0}\ncmd: {1}", groupEP.ToString(), cmd);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
            finally
            {
                listener.Close();
            }

            listener.Close();

            return cmd;
        }

        static bool tcpSendFile(string server, string path)
        {
            try
            {
                // Create a TcpClient.
                // Note, for this client to work you need to have a TcpServer 
                // connected to the same address as specified by the server, port
                // combination.
                Int32 port = 8999;
                TcpClient client = new TcpClient(server, port);

                Socket s = client.Client;

                if(s.Connected)
                {
                    s.SendFile(path);

                    client.Close();
                }
                else
                {
                    client.Close();

                    Console.WriteLine("Socket is not connected!!");

                    return false;
                }
            }
            catch (ArgumentNullException e)
            {
                Console.WriteLine("ArgumentNullException: {0}", e);
            }
            catch (SocketException e)
            {
                Console.WriteLine("SocketException: {0}", e);
            }

            return true;
        }

        static bool tcpRecvFile(string server, string path, int size)
        {
            try
            {
                // Create a TcpClient.
                // Note, for this client to work you need to have a TcpServer 
                // connected to the same address as specified by the server, port
                // combination.
                Int32 port = 8999;
                TcpClient client = new TcpClient(server, port);

                Socket s = client.Client;

                if (s.Connected)
                {
                    if (File.Exists(path))
                    {
                        File.Delete(path);
                    }

                    FileStream fs = File.Create(path);

                    int file_size = 0;

                    while (true)
                    {
                        byte[] bytes = new byte[64 * 1024];

                        int revCount = s.Receive(bytes);

                        if (revCount > 0)
                        {
                            fs.Write(bytes, 0, revCount);

                            file_size += revCount;

                            if (file_size == size)
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    s.Close();

                    fs.Close();

                    client.Close();
                }
                else
                {
                    client.Close();

                    Console.WriteLine("Socket is not connected!!");

                    return false;
                }
            }
            catch (ArgumentNullException e)
            {
                Console.WriteLine("ArgumentNullException: {0}", e);
            }
            catch (SocketException e)
            {
                Console.WriteLine("SocketException: {0}", e);
            }

            return true;
        }

        static void Main(string[] args)
        {
            string cmd = null;
            string opt = null;
            string name = null;
            string ip = null;
            int file_size = 0;

            while(true)
            {
                cmd = recvCmdForUdp();

                //Console.WriteLine("cmd: " + cmd);

                if (cmd.StartsWith("opt:"))
                {
                    string[] strArray = cmd.Split(';');

                    foreach(string str in strArray)
                    {
                        string[] temp = str.Split(':');

                        if (temp[0].Equals("opt"))
                        {
                            opt = temp[1];
                        }
                        else if (temp[0].Equals("name"))
                        {
                            name = temp[1];
                        }
                        else if (temp[0].Equals("ip"))
                        {
                            ip = temp[1];
                        }
                        else if (temp[0].Equals("size"))
                        {
                            file_size = int.Parse(temp[1]);
                        }
                    }
                }
                else
                {
                    Console.WriteLine("recv error, continue!!");

                    continue;
                }

                Console.WriteLine("opt: " + opt + " name: " + name + " ip: " + ip + " size: " + file_size);

                String path = Directory.GetCurrentDirectory() + "\\" + name;

                if ("g" == opt)
                {
                    if (File.Exists(path))
                    {
                        Console.WriteLine("tcp send file: " + path);

                        if (tcpSendFile(ip, path))
                        {
                            Console.WriteLine("文件传输结束！！！！");
                        }
                    }
                    else
                    {
                        Console.WriteLine("文件不存在： " + name);
                    }
                }
                else if ("p" == opt)
                {
                    Console.WriteLine("tcp recv file: " + path);

                    if (tcpRecvFile(ip, path, file_size))
                    {
                        Console.WriteLine("接收文件成功！！！！");
                    }
                }
            }
        }
    }
}

