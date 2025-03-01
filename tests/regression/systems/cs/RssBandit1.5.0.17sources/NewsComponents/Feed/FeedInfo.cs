#region CVS Version Header
/*
 * $Id: FeedInfo.cs,v 1.25 2007/07/18 00:07:19 carnage4life Exp $
 * Last modified by $Author: carnage4life $
 * Last modified at $Date: 2007/07/18 00:07:19 $
 * $Revision: 1.25 $
 */
#endregion

using System;
using System.Collections;
using System.Collections.Specialized;
using System.IO;
using System.Text;
using System.Xml;
using NewsComponents.Utils;

namespace NewsComponents.Feed
{
	/// <summary>
	/// represents information about a particular rss feed. 
	/// </summary>
	public class FeedInfo : FeedDetailsInternal, ISizeInfo
	{						 

		public static readonly FeedInfo Empty = new FeedInfo(String.Empty, String.Empty, new ArrayList(), String.Empty,String.Empty,String.Empty, new Hashtable(0), String.Empty);

		
		internal string id;
		/// <summary>Gets/sets the id of this feed</summary>
		public string Id{
			get { return id; }
			set { id = value; }
		}
		

		internal string feedLocation; //location in the cache not on the WWW
		public string FeedLocation {
			get { return feedLocation; }
			set { feedLocation = value; }
		}

		internal ArrayList itemsList; 

		public ArrayList ItemsList {
			get { return itemsList; }
			set { itemsList = value; }
		}
		internal Hashtable optionalElements; 

		/// <summary>
		/// Overloaded. Initializer
		/// </summary>
		/// <param name="id"></param>
		/// <param name="feedLocation"></param>
		/// <param name="itemsList"></param>
		public FeedInfo(string id, string feedLocation, ArrayList itemsList){
			this.id = id;
			this.feedLocation = feedLocation;  
			this.itemsList = itemsList; 
		}

		/// <summary>
		/// Overloaded. Initializer
		/// </summary>
		/// <param name="id"></param>
		/// <param name="feedLocation"></param>
		/// <param name="itemsList"></param>
		/// <param name="title"></param>
		/// <param name="link"></param>
		/// <param name="description"></param>
		public FeedInfo(string id, string feedLocation, ArrayList itemsList, string title, string link, string description)
			:this(id, feedLocation, itemsList, title, link, description, new Hashtable(), String.Empty){			
		}

		/// <summary>
		/// Overloaded. Initializer
		/// </summary>
		/// <param name="id"></param>
		/// <param name="feedLocation"></param>
		/// <param name="itemsList"></param>
		/// <param name="title"></param>
		/// <param name="link"></param>
		/// <param name="description"></param>
		/// <param name="optionalElements"></param>
		/// <param name="language"></param>
		public FeedInfo(string id, string feedLocation, ArrayList itemsList, string title, string link, string description, Hashtable optionalElements, string language){
			this.id = id;
			this.feedLocation = feedLocation; 
			this.itemsList = itemsList; 
			this.title = title; 
			this.link = link; 
			this.description = description; 
			this.optionalElements = optionalElements; 
			this.language = language; 

			if(RssHelper.IsNntpUrl(link)){
				this.type = FeedType.Nntp;	
			}else{
				this.type = FeedType.Rss;
			}
		}

		internal string title; 
		/// <summary></summary>
		public string Title{
			get { return title; }
		}

		internal string description; 
		/// <summary></summary>
		public string Description{
			get { return description; }
		}

		internal string link; 
		/// <summary></summary>
		public string Link{
			get { return link; }
		}

		internal string language; 
		/// <summary></summary>
		public string Language{
			get { return language; }
		}

		/// <summary>
		/// Table of optional feed elements.
		/// </summary>
		public Hashtable OptionalElements{
			get{ return this.optionalElements; }
		}

		internal FeedType type; 
		/// <summary>
		/// Gets the type of the FeedDetails
		/// </summary>
		public FeedType Type { get{ return this.type; } }


		/// <summary>
		/// Writes this object as an RSS 2.0 feed to the specified writer
		/// </summary>
		/// <param name="writer"></param>
		public void WriteTo(XmlWriter writer){
			this.WriteTo(writer, NewsItemSerializationFormat.RssFeed, true); 
		}

		/// <summary>
		/// Writes this object as an RSS 2.0 feed to the specified writer.
		/// </summary>
		/// <param name="writer"></param>
		/// <param name="noDescriptions">Indicates whether the contents of RSS items should 
		/// be written out or not.</param>
		public void WriteTo(XmlWriter writer, bool noDescriptions){
			this.WriteTo(writer, NewsItemSerializationFormat.RssFeed, true, noDescriptions); 
		}

		/// <summary>
		/// Writes this object as an RSS 2.0 feed to the specified writer
		/// </summary>
		/// <param name="writer"></param>
		/// <param name="format">indicates whether we are writing a FeedDemon newspaper or an RSS feed</param>
		public void WriteTo(XmlWriter writer, NewsItemSerializationFormat format){
			this.WriteTo(writer, format, true, false); 
		}

		/// <summary>
		/// Writes this object as an RSS 2.0 feed to the specified writer
		/// </summary>
		/// <param name="writer"></param>
		/// <param name="format">indicates whether we are writing a FeedDemon newspaper or an RSS feed</param>
		/// <param name="useGMTDate">Indicates whether the date should be GMT or local time</param>				
		public void WriteTo(XmlWriter writer, NewsItemSerializationFormat format, bool useGMTDate){
			this.WriteTo(writer, format, useGMTDate, false); 
		}

		

		/// <summary>
		/// Writes this object as an RSS 2.0 feed to the specified writer
		/// </summary>
		/// <param name="writer"></param>
		/// <param name="format">indicates whether we are writing a FeedDemon newspaper or an RSS feed</param>
		/// <param name="useGMTDate">Indicates whether the date should be GMT or local time</param>				
		/// <param name="noDescriptions">Indicates whether the contents of RSS items should 
		/// be written out or not.</param>
		public void WriteTo(XmlWriter writer, NewsItemSerializationFormat format, bool useGMTDate, bool noDescriptions){

			//writer.WriteStartDocument(); 

			if(format == NewsItemSerializationFormat.NewsPaper){
				//<newspaper type="channel">
				writer.WriteStartElement("newspaper"); 
				writer.WriteAttributeString("type", "channel"); 
				writer.WriteElementString("title", this.title); 
			}else if(format != NewsItemSerializationFormat.Channel){	
				//<rss version="2.0">
				writer.WriteStartElement("rss"); 
				writer.WriteAttributeString("version", "2.0"); 
			}

			/* These are here because so many people cut & paste into blogs from Microsoft Word 
			writer.WriteAttributeString("xmlns","v",null,"urn:schemas-microsoft-com:office:vml");
			writer.WriteAttributeString("xmlns","x",null,"urn:schemas-microsoft-com:office:excel");
			writer.WriteAttributeString("xmlns","o",null,"urn:schemas-microsoft-com:office:office");
			writer.WriteAttributeString("xmlns","w",null,"urn:schemas-microsoft-com:office:word");
			writer.WriteAttributeString("xmlns","st1",null,"urn:schemas-microsoft-com:office:smarttags");
			writer.WriteAttributeString("xmlns","st2",null,"urn:schemas-microsoft-com:office:smarttags");
			writer.WriteAttributeString("xmlns","asp",null,"http://www.example.com/asp");
			*/    

			//<channel>
			writer.WriteStartElement("channel"); 

			//<title />
			writer.WriteElementString("title", this.Title); 

			//<link /> 
			writer.WriteElementString("link", this.Link); 

			//<description /> 
			writer.WriteElementString("description", this.Description); 

			//<rssbandit:maxItemAge />
			//writer.WriteElementString("maxItemAge", "http://www.25hoursaday.com/2003/RSSBandit/feeds/", this.maxItemAge.ToString()); 

			//other stuff
			foreach(string s in this.optionalElements.Values){
				writer.WriteRaw(s); 	  
			}

			//<item />
			foreach(NewsItem item in this.itemsList){													
				writer.WriteRaw(item.ToString(NewsItemSerializationFormat.RssItem, useGMTDate, noDescriptions)); 					
			}
					
			writer.WriteEndElement();			
						
			if(format != NewsItemSerializationFormat.Channel){
				writer.WriteEndElement();
			}

			//writer.WriteEndDocument(); 
			
		}

		/// <summary>
		/// Provides the XML representation of the feed as an RSS 2.0 feed. 
		/// </summary>
		/// <param name="format">Indicates whether the XML should be returned as an RSS feed or a newspaper view</param>
		/// <returns>the feed as an XML string</returns>
		public string ToString(NewsItemSerializationFormat format){
			return this.ToString(format, true);
		}

		/// <summary>
		/// Provides the XML representation of the feed as an RSS 2.0 feed. 
		/// </summary>
		/// <param name="format">Indicates whether the XML should be returned as an RSS feed or a newspaper view</param>
		/// <param name="useGMTDate">Indicates whether the date should be GMT or local time</param>
		/// <returns>the feed as an XML string</returns>
		public string ToString(NewsItemSerializationFormat format,  bool useGMTDate){
				
			StringBuilder sb     = new StringBuilder("");
			XmlTextWriter writer = new XmlTextWriter(new StringWriter(sb)); 
				
			this.WriteTo(writer, format, useGMTDate); 
				
			writer.Flush(); 
			writer.Close(); 

			return sb.ToString(); 

		}


		/// <summary>
		/// Provides the XML representation of the feed as an RSS 2.0 feed. 
		/// </summary>
		/// <returns>the feed as an XML string</returns>
		public override string ToString(){
				return this.ToString(NewsItemSerializationFormat.RssFeed); 	
		}

		/// <summary>
		/// Returns a copy of this FeedInfo. The OptionalElements and ItemsList are only a shallow copies.
		/// </summary>
		/// <param name="includeNewsItems">if set to <c>true</c> the item list is cloned (shallow). If false, it is the empty list</param>
		/// <returns>A copy of this FeedInfo</returns>
		public FeedInfo Clone(bool includeNewsItems)
		{
			FeedInfo toReturn = new FeedInfo(this.id, this.feedLocation, 
				(includeNewsItems ? (ArrayList) this.itemsList.Clone(): new ArrayList() ), 
				this.title, this.link, this.description, 
				(Hashtable) this.optionalElements.Clone(), this.language); 

			return toReturn; 		
		}
		
		/// <summary>
		/// Returns a copy of this FeedInfo. The OptionalElements and ItemsList are only a shallow copies.
		/// </summary>
		/// <returns>A copy of this FeedInfo</returns>
		public object Clone()
		{
			return this.Clone(true); 		
		}


		/// <summary>
		/// Writes the union of the distinct item IDs and contents of the NewsItems contained 
		/// in the input reader to the specified binary writer. 
		/// </summary>
		/// <param name="reader">A reader positioned over the old descriptions file</param>
		/// <param name="writer"></param>
		public void WriteItemContents(BinaryReader reader, BinaryWriter writer){

			StringCollection inMemoryDescriptions = new StringCollection();
			//string id;
			byte[] content; 
			int count; 
		
			foreach(NewsItem item in this.itemsList){						
				if(item.HasContent){
					writer.Write(item.Id);  
					writer.Write(item.GetContent().Length);
					writer.Write(item.GetContent()); 			
					inMemoryDescriptions.Add(item.Id); 
				}				
			}//foreach(NewsItem...)
	
			if(reader != null){							
				id = reader.ReadString(); 
				
				while(!id.Equals(FileHelper.EndOfBinaryFileMarker)){
					count = reader.ReadInt32();
					content = reader.ReadBytes(count);
				
					if(!inMemoryDescriptions.Contains(id) && this.ContainsItemWithId(id)){
						writer.Write(id); 
						writer.Write(count); 
						writer.Write(content); 
					}
					id = reader.ReadString(); 
				}//while(!id.Equals(...))
			}//if(reader!= null) 


		}


		/// <summary>
		/// Determines whether a NewsItem with the specified ID is contained in this FeedInfo
		/// </summary>
		/// <param name="id">The ID of the NewsItem</param>
		/// <returns></returns>
		private bool ContainsItemWithId(string id){
		
			foreach(NewsItem item in this.ItemsList.ToArray()){
				if(item.Id.Equals(id)){
					return true; 
				}
			}
			return false; 
		}

		#region ISizeInfo Members

		/// <summary>
		/// Gets the size.
		/// </summary>
		/// <returns></returns>
		public int GetSize() {
			int iSize = StringHelper.SizeOfStr(this.link);
			iSize += StringHelper.SizeOfStr(this.title);
			iSize += StringHelper.SizeOfStr(this.title);
			iSize += StringHelper.SizeOfStr(this.description);
			//iSize += this.SizeOf(this.optionalElements);
			return iSize;
		}

		/// <summary>
		/// Gets the size details.
		/// </summary>
		/// <returns></returns>
		public string GetSizeDetails() {
			return this.GetSize().ToString();
		}

		#endregion
	}


	/// <summary>
	/// Represents a list of FeedInfo objects. This is primarily used for generating newspaper views of multiple feeds.
	/// </summary>
	public class FeedInfoList: IEnumerable, ICollection
	{

		#region Private Members

		/// <summary>
		/// The list of feeds
		/// </summary>
		private ArrayList feeds = new ArrayList();

		/// <summary>
		/// The title of this list when displayed in a newspaper view
		/// </summary>
		private string title;

		#endregion 

		#region Constructors 

		/// <summary>
		/// Creates a list with the specified title
		/// </summary>
		/// <param name="title">The name of the list</param>
		public FeedInfoList(string title){
			this.title = title; 
		}

		#endregion 

		#region Public properties

		/// <summary>
		/// Returns the name of the list
		/// </summary>
		public string Title{
			get { return this.title;}
		}

		#endregion 

		#region Public methods 


		/// <summary>
		/// Returns all the NewsItems contained within this FeedInfoList in an ArrayList
		/// </summary>
		/// <returns>a list of all the NewsItems in this FeedInfoList</returns>
		public ArrayList GetAllNewsItems(){
		
			ArrayList allItems = new ArrayList(); 

			foreach(FeedInfo fi in this.feeds){
				allItems.InsertRange(0, fi.ItemsList); 
			}

			return allItems; 
		}

		/// <summary>
		/// Adds a new Feed to the list
		/// </summary>
		/// <param name="feed">The FeedInfo object to add</param>
		/// <returns>The position into which the new feed was inserted</returns>
		public int Add(FeedInfo feed){
			return this.feeds.Add(feed);
		}
		/// <summary>
		/// Adds the range of feeds (FeedInfo collection).
		/// </summary>
		/// <param name="feedCollection">The feed collection.</param>
		public void AddRange(ICollection feedCollection)
		{
			this.feeds.AddRange(feedCollection);
		}

		/// <summary>
		/// Removes all FeedInfo objects from the list
		/// </summary>
		public void Clear(){
			this.feeds.Clear(); 
		}

		/// <summary>
		/// Gets the amount of FeedInfo objects.
		/// </summary>
		/// <value>The count.</value>
		public int NewsItemCount {
			get { 
				int count = 0; 

				foreach(FeedInfo fi in this.feeds){
					count += fi.ItemsList.Count; 
				}
				return count; 
			}
		}

		/// <summary>
		/// Gets the total amount of NewsItem objects in the FeedInfo objects held by this list.
		/// </summary>
		/// <value>The count.</value>
		public int Count {
			get { return this.feeds.Count; }
		}
		
		/// <summary>
		/// Provides the XML representation of the list as a FeedDemon newspaper 
		/// </summary>
		/// <returns>the feed list as an XML string</returns>
		public override string ToString(){
				
			StringBuilder sb     = new StringBuilder("");
			XmlTextWriter writer = new XmlTextWriter(new StringWriter(sb)); 
				
			this.WriteTo(writer); 
				
			writer.Flush(); 
			writer.Close(); 

			return sb.ToString(); 

		}


		/// <summary>
		/// Writes this object as a FeedDemon group newspaper to the specified writer
		/// </summary>
		/// <param name="writer"></param>
		public void WriteTo(XmlWriter writer){
									
			writer.WriteStartElement("newspaper"); 
			writer.WriteAttributeString("type", "group"); 
			writer.WriteElementString("title", this.title); 

			foreach(FeedInfo feed in this.feeds){
				feed.WriteTo(writer, NewsItemSerializationFormat.Channel, false); 
			}
			
			writer.WriteEndElement(); 

		}


		/// <summary>
		/// Returns an enumerator used to iterate over the FeedInfo objects in the list
		/// </summary>
		/// <returns></returns>
		public IEnumerator GetEnumerator(){
			return this.feeds.GetEnumerator(); 
		}

		#endregion 
	
	
		#region ICollection Members

		/// <summary>
		/// Copies the elements of the <see cref="T:System.Collections.ICollection"></see> to an <see cref="T:System.Array"></see>, starting at a particular <see cref="T:System.Array"></see> index.
		/// </summary>
		/// <param name="array">The one-dimensional <see cref="T:System.Array"></see> that is the destination of the elements copied from <see cref="T:System.Collections.ICollection"></see>. The <see cref="T:System.Array"></see> must have zero-based indexing.</param>
		/// <param name="index">The zero-based index in array at which copying begins.</param>
		/// <exception cref="T:System.ArgumentNullException">array is null. </exception>
		/// <exception cref="T:System.ArgumentOutOfRangeException">index is less than zero. </exception>
		/// <exception cref="T:System.ArgumentException">array is multidimensional.-or- index is equal to or greater than the length of array.-or- The number of elements in the source <see cref="T:System.Collections.ICollection"></see> is greater than the available space from index to the end of the destination array. </exception>
		/// <exception cref="T:System.InvalidCastException">The type of the source <see cref="T:System.Collections.ICollection"></see> cannot be cast automatically to the type of the destination array. </exception>
		public void CopyTo(Array array, int index)
		{
			feeds.CopyTo(array, index);
		}

		/// <summary>
		/// Gets a value indicating whether access to the <see cref="T:System.Collections.ICollection"></see> is synchronized (thread safe).
		/// </summary>
		/// <value></value>
		/// <returns>true if access to the <see cref="T:System.Collections.ICollection"></see> is synchronized (thread safe); otherwise, false.</returns>
		public bool IsSynchronized
		{
			get { return feeds.IsSynchronized; }
		}

		/// <summary>
		/// Gets an object that can be used to synchronize access to the <see cref="T:System.Collections.ICollection"></see>.
		/// </summary>
		/// <value></value>
		/// <returns>An object that can be used to synchronize access to the <see cref="T:System.Collections.ICollection"></see>.</returns>
		public object SyncRoot
		{
			get { return feeds.SyncRoot; }
		}

		#endregion
	}
}

#region CVS Version Log
/*
 * $Log: FeedInfo.cs,v $
 * Revision 1.25  2007/07/18 00:07:19  carnage4life
 * Fixed bug where we lost content of feed items no longer in the RSS feed
 *
 * Revision 1.24  2007/07/07 20:59:36  carnage4life
 * Fixed typo
 *
 * Revision 1.23  2007/07/07 20:58:41  carnage4life
 * Fixed issue where deleted items weren't being deleted from the cache
 *
 * Revision 1.22  2007/06/07 19:51:22  carnage4life
 * Added full support for pagination in newspaper views
 *
 * Revision 1.21  2007/02/15 16:37:49  t_rendelmann
 * changed: persisted searches now return full item texts;
 * fixed: we do now show the error of not supported search kinds to the user;
 *
 * Revision 1.20  2007/01/16 00:27:54  carnage4life
 * Made some perf improvements related to SearchNewsItems()
 *
 * Revision 1.19  2007/01/14 19:30:47  t_rendelmann
 * cont. SearchPanel: first main form integration and search working (scope/populate search scope tree is still a TODO)
 *
 * Revision 1.18  2006/10/27 19:15:43  t_rendelmann
 * added: Clone(bool) overload for speed opt.
 *
 * Revision 1.17  2006/08/18 19:10:57  t_rendelmann
 * added an "id" XML attribute to the feedsFeed. We need it to make the feed items (feeditem.id + feed.id) unique to enable progressive indexing (lucene)
 *
 */
#endregion
