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
using System.IO;
using System.Net;

namespace Starlight.NSC
{
    public class NSCParser
    {
        private const string FORMAT_SECTION_HEADER = "[Formats]";
        private const string ADDRESS_SECTION_HEADER = "[Address]";

        private const string KEY_MULTICAST_ADAPTER = "Multicast Adapter";
        private const string KEY_IP_ADDRESS = "IP Address";
        private const string KEY_IP_PORT = "IP Port";
        private const string KEY_NAME = "Name";

        private const string KEY_TTL = "Time To Live";
        private const string KEY_ECC = "Default Ecc";
        private const string KEY_LOGURL = "Log URL";
        private const string KEY_ROLLOVER = "Unicast URL";
        private const string KEY_BUFFER_TIME = "Network Buffer Time";

        private bool inAddressSection = false;
        private bool inFormatSection = false;
        public NSC ParseNSC(string nscContent)
        {
            NSC parsedNSC = new NSC();
            StringReader reader = new StringReader(nscContent);
            string currentLine = reader.ReadLine();
            while (currentLine != null)
            {
                string cleanLine = currentLine.Trim();
                if (cleanLine.Length == 0)
                {
                    //Skip blank lines.
                }
                else if (cleanLine.StartsWith("[") && cleanLine.EndsWith("]"))
                {
                    //Change our mode
                    inAddressSection = false;
                    inFormatSection = false;
                    if (cleanLine.Equals(ADDRESS_SECTION_HEADER))
                    {
                        inAddressSection = true;
                    }
                    else if (cleanLine.Equals(FORMAT_SECTION_HEADER))
                    {
                        inFormatSection = true;
                    }
                }
                else if (inAddressSection)
                {
                    HandleAddressLine(parsedNSC, cleanLine);
                }
                else if (inFormatSection)
                {
                    HandleFormatLine(parsedNSC, cleanLine);
                }

                currentLine = reader.ReadLine();
            }
            return parsedNSC;
        }

        private void HandleFormatLine(NSC nsc, string cleanLine)
        {
            string[] split = SplitLine(cleanLine);
            if(split[0].StartsWith("Format"))
            {
                nsc.AddHeader(ReadHeader(split[1]));
            }
        }

        private void HandleAddressLine(NSC nsc, string cleanLine)
        {
            string[] split = SplitLine(cleanLine);
            switch (split[0])
            {
                case KEY_NAME:
                    nsc.Name = ReadString(split[1]);
                    break;
                case KEY_MULTICAST_ADAPTER:
                    nsc.MulticastAdapter = ReadString(split[1]);
                    break;
                case KEY_IP_ADDRESS:
                    nsc.Address = ReadString(split[1]);
                    break;
                case KEY_IP_PORT:
                    nsc.Port = ReadInt(split[1]);
                    break;
                case KEY_TTL:
                    nsc.TTL = ReadInt(split[1]);
                    break;
                case KEY_ECC:
                    nsc.ECC = ReadInt(split[1]);
                    break;
                case KEY_BUFFER_TIME:
                    nsc.BufferTime = ReadInt(split[1]);
                    break;
                case KEY_LOGURL:
                    nsc.LogURL = ReadString(split[1]);
                    break;
                case KEY_ROLLOVER:
                    nsc.UnicastURL = ReadString(split[1]);
                    break;
                default:
                    break;
            }
        }

        private string ReadString(string value)
        {
            if(value.StartsWith("02"))
            {
                byte[] data = NSCBase64Decoder.Decode(value.Substring(2));
                BinaryReader reader = new BinaryReader(new MemoryStream(data));
                byte crc = reader.ReadByte();
                byte[] keyBytes = reader.ReadBytes(4);
                byte[] lengthBytes = reader.ReadBytes(4);
                Array.Reverse(lengthBytes);
                uint length = BitConverter.ToUInt32(lengthBytes, 0);
                string s = System.Text.Encoding.Unicode.GetString(data, 9, (int)length);
                if (s.EndsWith("\0"))
                {
                    s = s.Substring(0, s.Length - 1);
                }
                return s;
            }
            else
            {
                return value;
            }
        }

        private int ReadInt(string value)
        {
            return Convert.ToInt32(value, 16);
        }

        private NSCHeader ReadHeader(string value)
        {
            byte[] data = NSCBase64Decoder.Decode(value.Substring(2));
            BinaryReader reader = new BinaryReader(new MemoryStream(data));
            byte crc = reader.ReadByte();
            byte[] keyBytes = reader.ReadBytes(4);
            byte[] lengthBytes = reader.ReadBytes(4);
            Array.Reverse(lengthBytes);
            Array.Reverse(keyBytes);
            uint length = BitConverter.ToUInt32(lengthBytes, 0);
            uint format = BitConverter.ToUInt32(keyBytes, 0);
            byte[] headerData = new byte[length];
            Array.Copy(data, 9, headerData, 0, (int)length);
            NSCHeader h = new NSCHeader();
            h.ID = format;
            h.Data = headerData;
            return h;
        }

        private string[] SplitLine(string cleanLine)
        {
            int equalsIndex = cleanLine.IndexOf('=');
            if(equalsIndex == -1)
            {
                throw new NSCParseException("'=' not found in line: " + cleanLine);
            }
            string beforeEquals = cleanLine.Substring(0, equalsIndex);
            string afterEquals = "";
            if (equalsIndex < cleanLine.Length - 1)
            {
                afterEquals = cleanLine.Substring(equalsIndex + 1);
            }
            string[] ret = new string[2];
            ret[0] = beforeEquals.Trim();
            ret[1] = afterEquals.Trim();
            return ret;
        }

    }
}
