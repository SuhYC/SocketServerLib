using CSServerLib;
using System;
using System.Threading.Tasks;

class Program
{
    const int port = 12345;

    static void Main(string[] args)
    {
        GameServer server = new GameServer();

        server.Run(port);

        Console.ReadKey(true);

        Console.WriteLine($"Closing...");

        server.End().Wait();

        return;
    }
}