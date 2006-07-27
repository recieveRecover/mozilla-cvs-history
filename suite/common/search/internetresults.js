function searchResultsOpenURL(event, node)
{
	if (event.button != 1)
		return(false);

	if (node.name != "treeitem")
		return(false);

	var url = node.getAttribute('id');

	if (node.getAttribute('container') == "true")
	{
		return(false);
	}

	var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
	if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
	if (rdf)
	{
		var ds = rdf.GetDataSource("rdf:internetsearch");
		if (ds)
		{
			var src = rdf.GetResource(url, true);
			var prop = rdf.GetResource("http://home.netscape.com/NC-rdf#URL", true);
			var target = ds.GetTarget(src, prop, true);
			if (target)	target = target.QueryInterface(Components.interfaces.nsIRDFLiteral);
			if (target)	target = target.Value;
			if (target)	url = target;
			
		}
	}

	// Ignore "NC:" urls.
	if (url.substring(0, 3) == "NC:")
		return(false);

    top._content.location.href = url;

	return(true);
}



function onLoadInternetResults()
{
    // clear any previous results on load
	var iSearch = Components.classes["component://netscape/rdf/datasource?name=internetsearch"].getService();
	if (iSearch)      iSearch = iSearch.QueryInterface(Components.interfaces.nsIInternetSearchService);
	if (iSearch)      iSearch.ClearResultSearchSites();

    // the search URI is passed in as a parameter, so get it and them root the results tree
    var searchURI = top._content.location.href;
    if (searchURI)
    {
        var offset = searchURI.indexOf("?");
        if (offset > 0)
        {
            searchURI = searchURI.substr(offset+1);
            loadResultsTree(searchURI);
        }
    }
    return(true);
}



function loadResultsTree( aSearchURL )
{
  var resultsTree = document.getElementById( "internetresultstree" );
  if( !resultsTree )
    return false;
  resultsTree.setAttribute( "ref", aSearchURL );
  return true;
}



function doEngineClick( event, aNode )
{
	if (event.button != 1)
		return(false);

	var html = null;

	var resultsTree = document.getElementById("internetresultstree");
	var contentArea = document.getElementById("content");
	var splitter = document.getElementById("gray_horizontal_splitter");
	var engineURI = aNode.getAttribute("id");
	if (engineURI == "allEngines")
	{
		resultsTree.setAttribute("style", "display: table;");
		splitter.setAttribute("style","display: block;");
		contentArea.setAttribute("style", "height: 100; width: 100%;");
	}
	else
	{
		resultsTree.setAttribute("style", "display: none;");
		splitter.setAttribute("style","display: none");
		try
		{
			var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
			if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
			if (rdf)
			{
				var internetSearchStore = rdf.GetDataSource("rdf:internetsearch");
				if (internetSearchStore)
				{
					var src = rdf.GetResource(engineURI, true);
					var htmlProperty = rdf.GetResource("http://home.netscape.com/NC-rdf#HTML", true);
					html = internetSearchStore.GetTarget(src, htmlProperty, true);
					if ( html )	html = html.QueryInterface(Components.interfaces.nsIRDFLiteral);
					if ( html )	html = html.Value;
				}
			}
		}
		catch(ex)
		{
		}
	}

	if ( html )
	{
		var doc = window.frames[0].document;
		if (doc)
		{
    		doc.open("text/html", "replace");
	    	doc.writeln( html );
		    doc.close();
		}
	}
	else
	{
		window.frames[0].document.location = "chrome://communicator/locale/search/default.htm";
	}
}



function doResultClick(node)
{
	var theID = node.getAttribute("id");
	if (!theID)	return(false);

	try
	{
		var rdf = Components.classes["component://netscape/rdf/rdf-service"].getService();
		if (rdf)   rdf = rdf.QueryInterface(Components.interfaces.nsIRDFService);
		if (rdf)
		{
			var internetSearchStore = rdf.GetDataSource("rdf:internetsearch");
			if (internetSearchStore)
			{
				var src = rdf.GetResource(theID, true);
				var urlProperty = rdf.GetResource("http://home.netscape.com/NC-rdf#URL", true);
				var bannerProperty = rdf.GetResource("http://home.netscape.com/NC-rdf#Banner", true);
				var htmlProperty = rdf.GetResource("http://home.netscape.com/NC-rdf#HTML", true);

				var url = internetSearchStore.GetTarget(src, urlProperty, true);
				if (url)	url = url.QueryInterface(Components.interfaces.nsIRDFLiteral);
				if (url)	url = url.Value;
				if (url)
				{
					var statusNode = document.getElementById("status-button");
					if (statusNode)
					{
						statusNode.setAttribute("value", url);
					}
				}

				var banner = internetSearchStore.GetTarget(src, bannerProperty, true);
				if (banner)	banner = banner.QueryInterface(Components.interfaces.nsIRDFLiteral);
				if (banner)	banner = banner.Value;

				var target = internetSearchStore.GetTarget(src, htmlProperty, true);
				if (target)	target = target.QueryInterface(Components.interfaces.nsIRDFLiteral);
				if (target)	target = target.Value;
				if (target)
				{
					var text = "<HTML><HEAD><TITLE>Search</TITLE><BASE TARGET='_top'></HEAD><BODY><FONT POINT-SIZE='9'>";

					if (banner)
					{
						// add a </A> and a <BR> just in case
						text += banner + "</A><BR>";
					}
					text += target;
					text += "</FONT></BODY></HTML>"
					
					var doc = window.frames[0].document;
					doc.open("text/html", "replace");
					doc.writeln(text);
					doc.close();
				}
			}
		}
	}
	catch(ex)
	{
	}
	return(true);
}

