const nsILocalFile = Components.interfaces.nsILocalFile;
var prefix = "";

function cp(source, dest, followLinks, newName)
{
    try {
    var sourceFile = Components.classes["component://netscape/file/local"].
	createInstance(nsILocalFile);
    sourceFile.initWithPath(source);
    
    var destFile = Components.classes["component://netscape/file/local"].
	createInstance(nsILocalFile);
    destFile.initWithPath(dest);
    
    }
    catch (e) {
        dump("Could not create nsILocalFile\n");
    }

    try {

        if (! destFile.isDirectory())
        {
            dump("destination not a directory!\n");
            return;
        }
    }
    catch (e) {
        dump("error accessing dest");
    }

    try {
        if (followLinks)
        {
            sourceFile.copyToFollowingLinks(destFile, newName);
        }
        else
        {
            sourceFile.copyTo(destFile, newName);
        }
    }
    catch (e) {
        dump("error coping" + e + "\n");
    }
}


function mv(source, dest, followLinks, newName)
{
    try {
    var sourceFile = Components.classes["component://netscape/file/local"].
	createInstance(nsILocalFile);
    sourceFile.initWithPath(source);
    
    var destFile = Components.classes["component://netscape/file/local"].
	createInstance(nsILocalFile);
    destFile.initWithPath(dest);
    
    }
    catch (e) {
        dump("Could not create nsILocalFile\n");
    }

    try {

        if (! destFile.isDirectory())
        {
            dump("destination not a directory!\n");
            return;
        }
    }
    catch (e) {
        dump("error accessing dest");
    }

    try {
        if (followLinks)
        {
            sourceFile.moveToFollowingLinks(destFile, newName);
        }
        else
        {
            sourceFile.moveTo(destFile, newName);
        }
    }
    catch (e) {
        dump("error coping" + e + "\n");
    }
}
