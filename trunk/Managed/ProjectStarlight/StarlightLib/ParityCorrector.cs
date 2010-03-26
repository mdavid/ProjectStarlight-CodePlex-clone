using System;
using System.Net;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using Starlight.PacketSource;

namespace Starlight.Lib
{
    public class ParityCorrector
    {
        private static readonly int ERROR_CORRECTION_TYPE_XOR = 1;
        private static readonly int ERROR_CORRECTION_TYPE_PARITY = 2;
        private List<ECCPacket> cyclePackets = new List<ECCPacket>();
        private object sequenceLock = new object();
        private bool isNewCycle = true;
        private byte currentCycleId = 0;

        public List<Packet> AddPacket(Packet p, uint sequenceId)
        {
            lock (sequenceLock)
            {
                byte[] packetData = p.PacketData;
                byte b = packetData[0];

                bool hasErrorCorrectionData = (b & 0x80) == 0x80;
                if (hasErrorCorrectionData)
                {
                    bool isParityPacket = (b & 0x10) == 0x10;

                    //Error case.  We are starting a new cycle, but just got
                    //a parity packet.  Immediately return an result
                    //with no packets.
                    if (isParityPacket && isNewCycle)
                    {
                        System.Diagnostics.Debug.WriteLine("Got a parity packet when we expected a new cycle");
                        return null;
                    }
                   
                    //Parse the error correction data
                    int errorCorrectionDataLength = (b & 0x0F);

                    //We can only handle 2 byte error correction data
                    if (errorCorrectionDataLength != 2)
                    {
                        System.Diagnostics.Debug.WriteLine("Got error correction data length of " + errorCorrectionDataLength);
                        Reset();
                        return null;
                    }

                    byte errorCorrectionTypeAndNumber = packetData[1];

                    //Error correction type is "No Error Correction"
                    if (errorCorrectionTypeAndNumber == 0)
                    {
                        //Reset and return the current packet.
                        List<Packet> packetList = new List<Packet>();
                        packetList.Add(p);
                        return packetList;
                    }

                    int errorCorrectionType = (errorCorrectionTypeAndNumber & 0xF);

                    //Error case:  if this is a parity packet, type must specify so.
                    //In this case we will just ignore the packet and continue trying
                    //to validate this sequence.
                    if (isParityPacket && errorCorrectionType != ERROR_CORRECTION_TYPE_PARITY)
                    {
                        System.Diagnostics.Debug.WriteLine("Got parity packet with ecc type != parity");
                        Reset();
                        return null;
                    }

                    //If the number is 1 set the number to 
                    uint errorCorrectionNumber = (uint)( (errorCorrectionTypeAndNumber >> 4) & 0xF);
                    byte cycleId = packetData[2];

                    //Validate that the packet is part of the current cycle
                    if (isNewCycle)
                    {
                        currentCycleId = cycleId;
                    }
                    else
                    {
                        if (currentCycleId != cycleId)
                        {
                            System.Diagnostics.Debug.WriteLine("Got packet in new cycle before parity packet, probable dropped parity packet");
                            
                            //The packets might not be valid, but we'll let the ASF parser sort that out. 
                            List<Packet> packetList = new List<Packet>();
                            foreach (ECCPacket eccPacket in cyclePackets)
                            {
                                packetList.Add(eccPacket.packet);
                            }
                            packetList.Add(p);
                            Reset();
                            return packetList;
                        }
                    }

                    if (isParityPacket)
                    {
                        return PerformErrorCorrectionAndReset(p, sequenceId);
                    }
                    else
                    {
                        isNewCycle = false;
                        cyclePackets.Add(new ECCPacket(p, sequenceId, errorCorrectionNumber));
                        return null;
                    }
                }
                else
                {
                    //We got a packet with no ECC Data.  Return a 
                    //sequence of just the passed in packet.
                    List<Packet> packetList = new List<Packet>();
                    packetList.Add(p);
                    return packetList;
                }
            }

        }

        private List<Packet> PerformErrorCorrectionAndReset(Packet parityPacket, uint sequenceId)
        {
            try
            {
                byte[] packetData = parityPacket.PacketData;

                //If there are no packets in this cycle that's an error
                if (cyclePackets.Count == 0)
                {
                    System.Diagnostics.Debug.WriteLine("Found empty ecc cycle");
                    return null;
                }

                //Calculate the parity
                byte[] parityData = new byte[packetData.Length - 3];
                bool parityOk = true;
                for (int i = 3; i < packetData.Length; i++)
                {
                    byte xor = 0;
                    foreach (ECCPacket eccPacket in cyclePackets)
                    {
                        byte b = 0;
                        if (i < eccPacket.packet.PacketData.Length)
                        {
                            b = eccPacket.packet.PacketData[i];
                        }
                        xor ^= b;
                    }
                    parityData[i - 3] = xor;
                    if (parityData[i - 3] != packetData[i])
                    {
                        parityOk = false;
                    }
                }

                List<Packet> packets = new List<Packet>(cyclePackets.Count);
                foreach (ECCPacket eccPacket in cyclePackets)
                {
                    packets.Add(eccPacket.packet);
                }

                //If the parity didn't match try to do correction
                if(!parityOk)
                {
                    //Find the number of missing packets.  Use the sequence number
                    //instead of the ecc cycle number since the cycle number could 
                    //set all packets to 1
                    uint lastSequenceId = cyclePackets[0].sequenceId;
                    int missingPacketIndex = 0;
                    int missingPacketCount = 0;
                    for (int i = 1; i < cyclePackets.Count; i++)
                    {
                        if (cyclePackets[i].sequenceId != lastSequenceId + 1)
                        {
                            missingPacketIndex = i;
                            int missingPackets = (int)(cyclePackets[i].sequenceId - lastSequenceId - 1);
                            missingPacketCount += missingPackets;
                            //System.Diagnostics.Debug.WriteLine("Missing " + missingPackets + " at seq " + cyclePackets[i].sequenceId);
                        }
                        lastSequenceId = cyclePackets[i].sequenceId;
                    }

                    //check for missing packets at the end.  The parity packet's sequenceId should == the last data packet's 
                    //sequence id
                    if (sequenceId != lastSequenceId)
                    {
                        missingPacketIndex = cyclePackets.Count;
                        int missingPackets = (int)(sequenceId - lastSequenceId);
                        missingPacketCount += missingPackets;
                        //System.Diagnostics.Debug.WriteLine("Missing " + missingPackets + " at seq " + sequenceId);
                    }

                    //check if we are missing packets at the beginning if the server is properly setting 
                    //ecc cycle numbers.
                    missingPacketCount += (int)(cyclePackets[0].errorSequenceNumber - 1);

                    //If there was no packet loss but a parity error we have corruption and should throw away the
                    //whole cycle.
                    if (missingPacketCount == 0)
                    {
                        System.Diagnostics.Debug.WriteLine("Found a parity error dropping sequence starting at " + cyclePackets[0].sequenceId);
                        return null;
                    }

                    //If there was more than 1 missing packet we can't do correction, but return the packets we did get.
                    //This will probably be invalid, but the ASF parser will sort this out.
                    if (missingPacketCount > 1)
                    {
                        System.Diagnostics.Debug.WriteLine("Returning uncorrected ecc cycle since more than one packet was lost");
                    }
                    else
                    {
                        //Only one packet was lost, so calculate the missing packet data
                        for (int i = 3; i < packetData.Length; i++)
                        {
                            parityData[i - 3] ^= packetData[i];
                        }

                        //Make a new Packet with the parity data.
                        Packet reconstructedPacket = new Packet();
                        reconstructedPacket.PacketData = parityData;
                        reconstructedPacket.PacketHeader = parityPacket.PacketHeader;
                        packets.Insert(missingPacketIndex, reconstructedPacket);
                    }
                }
                return packets;
            }
            finally
            {
                Reset();
            }
        }

        private void Reset()
        {
            isNewCycle = true;
            cyclePackets.Clear();
            currentCycleId = 0;
        }


        private class ECCPacket
        {
            public readonly Packet packet;
            public readonly uint sequenceId;
            public readonly uint errorSequenceNumber;
            public ECCPacket(Packet packet, uint sequenceId, uint errorSequenceNumber)
            {
                this.packet = packet;
                this.sequenceId = sequenceId;
                this.errorSequenceNumber = errorSequenceNumber;
            }
        }
    }
}
