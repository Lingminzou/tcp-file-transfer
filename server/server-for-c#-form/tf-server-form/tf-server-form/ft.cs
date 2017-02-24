using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace tf_server_form
{
    class ft
    {
        static string file_path;

        Thread th = null;

        public ft()
        {
            file_path = Directory.GetCurrentDirectory();

            th = new Thread(down_thread);

            th.IsBackground = true;

            th.Start();
        }

        ~ft()
        {
            th.Abort();
        }

        static public void setPath(string str)
        {
            file_path = str;
        }

        static string recvCmdForUdp()
        {
            const int listenPort = 8899;
            string cmd = null;

            UdpClient listener = new UdpClient(listenPort);
            IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);

            try
            {
                Debug.WriteLine("Waiting for broadcast........");

                byte[] bytes = listener.Receive(ref groupEP);

                cmd = Encoding.ASCII.GetString(bytes, 0, bytes.Length);

                Debug.WriteLine("Received broadcast from {0}", groupEP.ToString());

                //Console.WriteLine("Received broadcast from {0}\ncmd: {1}", groupEP.ToString(), cmd);
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.ToString());
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

                if (s.Connected)
                {
                    s.SendFile(path);

                    client.Close();
                }
                else
                {
                    client.Close();

                    Debug.WriteLine("Socket is not connected!!");

                    return false;
                }
            }
            catch (ArgumentNullException e)
            {
                Debug.WriteLine("ArgumentNullException: {0}", e);
            }
            catch (SocketException e)
            {
                Debug.WriteLine("SocketException: {0}", e);
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

        static void down_thread()
        {
            string cmd = null;
            string opt = null;
            string name = null;
            string ip = null;
            int file_size = 0;

            while (true)
            {
                cmd = recvCmdForUdp();

                if (cmd.StartsWith("opt:"))
                {
                    string[] strArray = cmd.Split(';');

                    foreach (string str in strArray)
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
                    Debug.WriteLine("recv error, continue!!");

                    continue;
                }

                Debug.WriteLine("opt: " + opt + " name: " + name + " ip: " + ip + " size: " + file_size);

                String path = file_path + "\\" + name;

                if ("g" == opt)
                {
                    if (File.Exists(path))
                    {
                        Debug.WriteLine("tcp send file: " + path);

                        if (tcpSendFile(ip, path))
                        {
                            Debug.WriteLine("文件传输结束！！！！");
                        }
                    }
                    else
                    {
                        Debug.WriteLine("文件不存在： " + name);
                    }
                }
                else if ("p" == opt)
                {
                    Debug.WriteLine("tcp recv file: " + path);

                    if (tcpRecvFile(ip, path, file_size))
                    {
                        Debug.WriteLine("接收文件成功！！！！");
                    }
                }
            }
        }
    }
}
