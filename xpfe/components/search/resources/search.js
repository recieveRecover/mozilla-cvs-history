
var	gSearchStr = "";

function loadPage(thePage, searchStr)
{
	var	content="", results="";

	gSearchStr = "";

	if (thePage == "find")
	{
		content="find.xul";
		results="findresults.xul";
	}
	else if (thePage == "internet")
	{
		content="internet.xul";
		results="internetresults.xul";

		if ((searchStr) && (searchStr != null))
		{
			gSearchStr = searchStr;
		}
	}
	else if (thePage == "mail")
	{
		content="about:blank";
		results="about:blank";
	}
	else if (thePage == "addressbook")
	{
		content="about:blank";
		results="about:blank";
	}

	if ((content != "") && (results != ""))
	{
		var	contentFrame = document.getElementById("content");
		if (contentFrame)
		{
			contentFrame.setAttribute("src", content);
		}
		var	resultsFrame = document.getElementById("results");
		if (resultsFrame)
		{
			resultsFrame.setAttribute("src", results);
		}
	}
	return(true);
}



function getSearchText()
{
	return(gSearchStr);
}