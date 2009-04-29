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
using System.Xml;
using System.Linq;
using System.Xml.Linq;

namespace Starlight.Lib
{
    /// <summary>
    /// Parses an ASX metafile.
    /// </summary>
    public class ASXParser : PlaylistParser
    {
        private List<PlaylistEntry> playlistEntries = new List<PlaylistEntry>();
        private List<string[]>.Enumerator entryEnumerator;

        private PlaylistEntryFactory playlistEntryFactory;

        public ASXParser(PlaylistEntryFactory playlistEntryFactory)
        {
            this.playlistEntryFactory = playlistEntryFactory;
        }

        public override void ParsePlaylistAsync(string playlistData)
        {
            List<string[]> entries = new List<string[]>();
            XElement plXml = XElement.Parse(playlistData);
            foreach(XElement entry in plXml.Elements())
            {
                if(entry.Name.LocalName.ToLower().Equals("entry"))
                {
                    List<string> uris = new List<string>();
                    foreach (XElement refElem in entry.Elements())
                    {
                        if (refElem.Name.LocalName.ToLower().Equals("ref"))
                        {
                            uris.Add(refElem.Attribute("href").Value);
                        }
                    }
                    entries.Add(uris.ToArray());
                }

            }
            entryEnumerator = entries.GetEnumerator();

            OnEntryInitComplete();
        }

        private void OnEntryInitComplete()
        {
            if (entryEnumerator.MoveNext())
            {
                string[] mediaUris = entryEnumerator.Current;
                List<PlaylistEntry> childEntries = new List<PlaylistEntry>();
                foreach (string uri in mediaUris)
                {
                    PlaylistEntry pe = playlistEntryFactory.CreatePlaylistEntry(uri);
                    childEntries.Add(pe);
                }
                PlaylistEntry wrapper = new FailoverPlaylistEntry(childEntries.ToArray());
                playlistEntries.Add(wrapper);
                wrapper.InitCompleted += new PlaylistEntry.OnInitCompletedEventHandler(this.OnEntryInitComplete);
                wrapper.InitAsync();
            }
            else
            {
                this.OnParsePlaylistCompleted(new Playlist(playlistEntries.ToArray()));
            }
        }
    }


}
