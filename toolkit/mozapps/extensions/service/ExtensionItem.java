package org.mozilla.update.extensions;

public class ExtensionItem
{
  private int row;
  private java.lang.String id;
  private java.lang.String version;
  private java.lang.String name;
  private java.lang.String xpiURL;
  private java.lang.String iconURL;
  private int type;

  public ExtensionItem() 
  {
  }

  public int getRow() 
  {
    return row;
  }

  public void setRow(int row) 
  {
    this.row = row;
  }

  public java.lang.String getId() 
  {
    return id;
  }

  public void setId(java.lang.String id) 
  {
    this.id = id;
  }

  public java.lang.String getVersion() 
  {
    return version;
  }

  public void setVersion(java.lang.String version) 
  {
    this.version = version;
  }

  public java.lang.String getName() 
  {
    return name;
  }

  public void setName(java.lang.String name) 
  {
    this.name = name;
  }

  public java.lang.String getXpiURL() 
  {
    return xpiURL;
  }

  public void setXpiURL(java.lang.String xpiURL) 
  {
    this.xpiURL = xpiURL;
  }
  
  public java.lang.String getIconURL() 
  {
    return iconURL;
  }

  public void setIconURL(java.lang.String iconURL) 
  {
    this.iconURL = iconURL;
  }
  
  public int getType() 
  {
    return type;
  }

  public void setType(int type) 
  {
    this.type = type;
  }
}

//public class ExtensionType
//{
//  public ExtensionType()
//  {
//  }
//
//  public int row;
//  public String id;
//  public String version;
//  public String name;
//  public String xpiURL;
//  public int type;
//}

