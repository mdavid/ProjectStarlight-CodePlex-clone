#region License Header
/**
* 
* This license governs use of the accompanying software. If you use the software, you
* accept this license. If you do not accept the license, do not use the software.
*
* 1. Definitions
* The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
* same meaning here as under U.S. copyright law.
* A "contribution" is the original software, or any additions or changes to the software.
* A "contributor" is any person that distributes its contribution under this license.
* "Licensed patents" are a contributor's patent claims that read directly on its contribution.
* 
* 2. Grant of Rights
* (A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
* (B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
* 
* 3. Conditions and Limitations
* (A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
* (B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
* (C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
* (D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
* (E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
*
*/
#endregion

using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using System.Threading;
using System.Windows.Browser;
using System.Windows.Threading;
using Starlight.PacketSource;

namespace Starlight.Lib
{
    /// <summary>
    /// Implements a packet source that has MSB packets pushed to it from an external source.
    ///
    /// </summary>
    [ScriptableType]
    public class MSBPushPacketSource : PacketSource.PacketSource
    {
        private static readonly byte[] BEACON_HEADER = new byte[] { 0x20, 0x42, 0x53, 0x4D };

        private List<Header> headers = new List<Header>();
        private uint lastPacketId = 0;
        private uint lastStreamId = 0;
        private volatile bool closed = false;
        private long lastPacketTime = 0;
        private long beaconCount = 0;
        private DispatcherTimer timer = new DispatcherTimer();
        private ParityCorrector parityCorrector = new ParityCorrector();

        private Random random = new Random();

        
        public MSBPushPacketSource(NSC.NSC nsc)
        {
            foreach (NSC.NSCHeader nscHeader in nsc.Headers)
            {
                Header h = new Header();
                h.HeaderContent = nscHeader.Data;
                h.HeaderId = nscHeader.ID;
                headers.Add(h);
            }
        }

        public Header[] AllHeaders
        {
            get { return headers.ToArray(); }
        }

        public void StartTimer()
        {
            lastPacketTime = DateTime.Now.Ticks;
            timer.Interval = new TimeSpan(0, 0, 5);
            timer.Tick += OnTimer;
            timer.Start();
        }

        public void StopTimer()
        {
            timer.Stop();
            timer = new DispatcherTimer();
        }


        /// <summary>
        /// Adds a MSB packet 
        /// 
        /// </summary>
        /// <param name="base64Content"></param>
        public void AddPacketBase64(string base64Content)
        {
            
            byte[] bytes = System.Convert.FromBase64String(base64Content);
            BinaryReader structReader = new BinaryReader(new MemoryStream(bytes));
            int packetCount = structReader.ReadInt32();
            //System.Diagnostics.Debug.WriteLine("Packet count: " + packetCount);
            for (uint i = 0; i < packetCount; i++)
            {

                int packetLength = structReader.ReadInt32();
                byte[] packetData = structReader.ReadBytes(packetLength);

                //Look for a beacon packet
                if(packetData.Length >= 4 
                        && packetData[3] == BEACON_HEADER[0]
                        && packetData[2] == BEACON_HEADER[1]
                        && packetData[1] == BEACON_HEADER[2]
                        && packetData[0] == BEACON_HEADER[3]) 
                {
                    System.Diagnostics.Debug.WriteLine("Dropped beacon packet");
                    beaconCount++;

                    //If we got some packets, and then at least 2 beacons in a row the 
                    //stream is ended, so fire null packets to indicate this.
                    if (beaconCount == 2 && lastPacketTime > 0)
                    {
                        foreach (Header h in AllHeaders)
                        {
                            Packet p = new Packet();
                            p.PacketData = null;
                            p.PacketHeader = h;
                            ReportPacketReceived(p);
                        }
                    }
                    continue;
                }
                
                //It's not a beacon, try to parse it
                Packet packet = new Packet();
                if (packetData.Length < 8)
                {
                    System.Diagnostics.Debug.WriteLine("Dropped too short packet");
                    continue;
                }
                
                byte[] headerBytes = new byte[8];
                System.Array.Copy(packetData, headerBytes, 8);
                beaconCount = 0;
                lastPacketTime = DateTime.Now.Ticks;
                BinaryReader headerReader = new BinaryReader(new MemoryStream(headerBytes));
                uint packetId = headerReader.ReadUInt32();
                uint msbStreamId = headerReader.ReadUInt16();
                msbStreamId = (msbStreamId & 0x7FF);

                //DEBUG CODE, UNCOMMENT TO SIMULATE PACKET LOSS
                //if (random.Next(100) < 1)
                //{
                //    System.Diagnostics.Debug.WriteLine("Dropping packet " + packetId);
                //    continue;
                //}
                //END DEBUG CODE

                if (packetId < lastPacketId)
                {
                    System.Diagnostics.Debug.WriteLine("Dropped out of seq packet");
                    continue;
                }

                if (lastPacketId > 0)
                {
                    uint packetDelta = packetId - lastPacketId;
                    if (packetDelta > 1)
                    {
                        System.Diagnostics.Debug.WriteLine("Packet delta: " + packetDelta);
                    }
                }
                lastPacketId = packetId;

                uint packetSize = headerReader.ReadUInt16();
                int dataLen = packetData.Length - 8;
                if (packetSize != packetData.Length)
                {
                    System.Diagnostics.Debug.WriteLine("Warning:  reported packet size != packet size");
                    //continue;
                }
                byte[] bodyBytes = new byte[dataLen];

                System.Array.Copy(packetData, 8, bodyBytes, 0, dataLen);

                foreach (Header h in headers)
                {
                    if (h.HeaderId == msbStreamId)
                    {
                        packet.PacketHeader = h;
                    }
                }
                
                packet.PacketData = bodyBytes;

                List<Packet> correctedPackets = parityCorrector.AddPacket(packet, packetId);
                if (correctedPackets != null)
                {
                    foreach (Packet p in correctedPackets)
                    {
                        ReportPacketReceived(p);
                    }
                }
            }
        }

        private void OnTimer(object sender, EventArgs e)
        {
            long now = DateTime.Now.Ticks;
            if(now - lastPacketTime > 30 * 10000 * 1000) 
            {
                foreach(Header h in AllHeaders)
                {
                    Packet p = new Packet();
                    p.PacketData = null;
                    p.PacketHeader = h;
                    ReportPacketReceived(p);
                }
            }
        }
    }
}
