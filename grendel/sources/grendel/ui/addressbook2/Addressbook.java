/*
 * Addressbook.java
 *
 * Created on 08 October 2005, 15:17
 */

package grendel.ui.addressbook2;

import grendel.addressbook.AddressBook;
import grendel.addressbook.AddressBookManager;
import grendel.addressbook.AddressCard;
import grendel.addressbook.AddressCardList;
import java.awt.Component;
import java.io.IOException;
import javax.swing.AbstractListModel;
import javax.swing.DefaultListCellRenderer;
import javax.swing.Icon;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.ListModel;
import javax.swing.UIManager;
import org.jdesktop.swing.JXTreeTable;
public class Addressbook extends javax.swing.JFrame
{
  
  /** Creates new form Addressbook */
  public Addressbook()
  {
    setIconImage(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/addressbook.png")).getImage());
    initComponents();
    //jxTreeTable = new JXTreeTable();
    class IconListCellRenderer extends DefaultListCellRenderer
    {
      private Icon icon;
      
      public IconListCellRenderer(Icon icon)
      {
        super();
        this.icon = icon;
      }
      
      public Component getListCellRendererComponent
          (JList list, Object value, int index,
          boolean isSelected,boolean cellHasFocus)
      {
        JLabel component = (JLabel) super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
        component.setIcon(icon);
        component.setHorizontalAlignment(JLabel.LEFT);
        return component;
      }
    }
    jList2.setCellRenderer(new IconListCellRenderer(new javax.swing.ImageIcon(Addressbook.class.getResource("/grendel/ui/addressbook2/abcard.png"))));
    jList1.setCellRenderer(new IconListCellRenderer(new javax.swing.ImageIcon(Addressbook.class.getResource("/grendel/ui/addressbook2/remote-addrbook.png"))));
    jxTreeTable = new JXTreeTable();
    jScrollPane3.setViewportView(jxTreeTable);
    jxTreeTable.setShowsRootHandles(true);
  }
  
  /** This method is called from within the constructor to
   * initialize the form.
   * WARNING: Do NOT modify this code. The content of this method is
   * always regenerated by the Form Editor.
   */
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        jSplitPane1 = new javax.swing.JSplitPane();
        jScrollPane1 = new javax.swing.JScrollPane();
        jList1 = new javax.swing.JList();
        jSplitPane2 = new javax.swing.JSplitPane();
        jScrollPane2 = new javax.swing.JScrollPane();
        jList2 = new javax.swing.JList();
        jScrollPane3 = new javax.swing.JScrollPane();
        jPanel1 = new javax.swing.JPanel();
        jPanel3 = new javax.swing.JPanel();
        jPanel2 = new javax.swing.JPanel();
        jLabel1 = new javax.swing.JLabel();
        jTextField1 = new javax.swing.JTextField();
        jButton1 = new javax.swing.JButton();
        jToolBar1 = new javax.swing.JToolBar();
        jButton2 = new javax.swing.JButton();
        jButton4 = new javax.swing.JButton();
        jButton6 = new javax.swing.JButton();
        jButton3 = new javax.swing.JButton();
        jButton5 = new javax.swing.JButton();
        jButton7 = new javax.swing.JButton();
        jMenuBar1 = new javax.swing.JMenuBar();
        jMenu1 = new javax.swing.JMenu();

        setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        setTitle("Grendel Addressbook 2");
        jSplitPane1.setOneTouchExpandable(true);
        jList1.setModel(getAddressBooks());
        jList1.setMinimumSize(new java.awt.Dimension(100, 100));
        jList1.addListSelectionListener(new javax.swing.event.ListSelectionListener() {
            public void valueChanged(javax.swing.event.ListSelectionEvent evt) {
                jList1ValueChanged(evt);
            }
        });

        jScrollPane1.setViewportView(jList1);

        jSplitPane1.setLeftComponent(jScrollPane1);

        jSplitPane2.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
        jSplitPane2.setOneTouchExpandable(true);
        jList2.addListSelectionListener(new javax.swing.event.ListSelectionListener() {
            public void valueChanged(javax.swing.event.ListSelectionEvent evt) {
                jList2ValueChanged(evt);
            }
        });

        jScrollPane2.setViewportView(jList2);

        jSplitPane2.setLeftComponent(jScrollPane2);

        jSplitPane2.setRightComponent(jScrollPane3);

        jSplitPane1.setRightComponent(jSplitPane2);

        jPanel1.setLayout(new java.awt.BorderLayout());

        jPanel3.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.LOWERED));
        org.jdesktop.layout.GroupLayout jPanel3Layout = new org.jdesktop.layout.GroupLayout(jPanel3);
        jPanel3.setLayout(jPanel3Layout);
        jPanel3Layout.setHorizontalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(0, 582, Short.MAX_VALUE)
        );
        jPanel3Layout.setVerticalGroup(
            jPanel3Layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(0, 15, Short.MAX_VALUE)
        );

        jPanel2.setLayout(new java.awt.BorderLayout(10, 0));

        jLabel1.setText("Search:");
        jPanel2.add(jLabel1, java.awt.BorderLayout.WEST);

        jTextField1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jTextField1ActionPerformed(evt);
            }
        });

        jPanel2.add(jTextField1, java.awt.BorderLayout.CENTER);

        jButton1.setText("Search");
        jButton1.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                jButton1ActionPerformed(evt);
            }
        });

        jPanel2.add(jButton1, java.awt.BorderLayout.EAST);

        jToolBar1.setFloatable(false);
        jToolBar1.setRollover(true);
        jButton2.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/address.png")));
        jButton2.setText("jButton2");
        jButton2.setBorderPainted(false);
        jButton2.setContentAreaFilled(false);
        jButton2.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton2);
        jButton2.getAccessibleContext().setAccessibleName("");

        jButton4.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/addressbook.png")));
        jButton4.setText("jButton4");
        jButton4.setBorderPainted(false);
        jButton4.setContentAreaFilled(false);
        jButton4.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton4);
        jButton4.getAccessibleContext().setAccessibleName("");

        jButton6.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/abcard-large.png")));
        jButton6.setText("jButton5");
        jButton6.setBorderPainted(false);
        jButton6.setContentAreaFilled(false);
        jButton6.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton6);
        jButton6.getAccessibleContext().setAccessibleName("");

        jButton3.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/save.png")));
        jButton3.setText("jButton3");
        jButton3.setBorderPainted(false);
        jButton3.setContentAreaFilled(false);
        jButton3.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton3);
        jButton3.getAccessibleContext().setAccessibleName("");

        jButton5.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/themeGeneric.png")));
        jButton5.setText("jButton5");
        jButton5.setBorderPainted(false);
        jButton5.setContentAreaFilled(false);
        jButton5.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton5);
        jButton5.getAccessibleContext().setAccessibleName("");

        jButton7.setIcon(new javax.swing.ImageIcon(getClass().getResource("/grendel/ui/addressbook2/stop.png")));
        jButton7.setText("jButton5");
        jButton7.setBorderPainted(false);
        jButton7.setContentAreaFilled(false);
        jButton7.setHorizontalTextPosition(javax.swing.SwingConstants.RIGHT);
        jToolBar1.add(jButton7);
        jButton7.getAccessibleContext().setAccessibleName("");

        jMenu1.setText("Menu");
        jMenuBar1.add(jMenu1);

        setJMenuBar(jMenuBar1);

        org.jdesktop.layout.GroupLayout layout = new org.jdesktop.layout.GroupLayout(getContentPane());
        getContentPane().setLayout(layout);
        layout.setHorizontalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(org.jdesktop.layout.GroupLayout.LEADING, layout.createSequentialGroup()
                .add(545, 545, 545)
                .add(jPanel1, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 41, Short.MAX_VALUE))
            .add(org.jdesktop.layout.GroupLayout.TRAILING, jPanel3)
            .add(org.jdesktop.layout.GroupLayout.TRAILING, layout.createSequentialGroup()
                .addContainerGap()
                .add(jPanel2, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 566, Short.MAX_VALUE)
                .addContainerGap())
            .add(jSplitPane1, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 586, Short.MAX_VALUE)
            .add(jToolBar1, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 586, Short.MAX_VALUE)
        );
        layout.setVerticalGroup(
            layout.createParallelGroup(org.jdesktop.layout.GroupLayout.LEADING)
            .add(org.jdesktop.layout.GroupLayout.LEADING, layout.createSequentialGroup()
                .add(jPanel1, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .add(0, 0, 0)
                .add(jToolBar1)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jPanel2, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE)
                .addPreferredGap(org.jdesktop.layout.LayoutStyle.RELATED)
                .add(jSplitPane1, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, 353, Short.MAX_VALUE)
                .add(0, 0, 0)
                .add(jPanel3, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE, org.jdesktop.layout.GroupLayout.DEFAULT_SIZE, org.jdesktop.layout.GroupLayout.PREFERRED_SIZE))
        );
        pack();
    }
    // </editor-fold>//GEN-END:initComponents
  
  private void jTextField1ActionPerformed(java.awt.event.ActionEvent evt)//GEN-FIRST:event_jTextField1ActionPerformed
  {//GEN-HEADEREND:event_jTextField1ActionPerformed
    jButton1ActionPerformed(evt);
  }//GEN-LAST:event_jTextField1ActionPerformed
  
  private void jButton1ActionPerformed(java.awt.event.ActionEvent evt)//GEN-FIRST:event_jButton1ActionPerformed
  {//GEN-HEADEREND:event_jButton1ActionPerformed
    String search = jTextField1.getText();
    if (search.equals("")) search = null;
    AddressBook book = (AddressBook) jList1.getSelectedValue();
    if (book!=null)
    {
      try
      {
        if (!book.isConnected()) book.connect();
        AddressCardList cl = book.getAddressCardList(search);
        class Model extends AbstractListModel
        {
          private AddressCardList l;
          public Model(AddressCardList l)
          {
            this.l = l;
          }
          /**
           * Returns the value at the specified index.
           * @param index the requested index
           * @return the value at <code>index</code>
           */
          public Object getElementAt(int index)
          {
            return l.get(index);
          }
          
          /**
           *
           * Returns the length of the list.
           * @return the length of the list
           */
          public int getSize()
          {
            return l.size();
          }
        }
        jList2.setModel(new Model(cl));
      }
      catch (IOException ex)
      {
        ex.printStackTrace();
      }
    }
  }//GEN-LAST:event_jButton1ActionPerformed
  
  private void jList1ValueChanged(javax.swing.event.ListSelectionEvent evt)//GEN-FIRST:event_jList1ValueChanged
  {//GEN-HEADEREND:event_jList1ValueChanged
    jButton1ActionPerformed(null);
  }//GEN-LAST:event_jList1ValueChanged
  
  private void jList2ValueChanged(javax.swing.event.ListSelectionEvent evt)//GEN-FIRST:event_jList2ValueChanged
  {//GEN-HEADEREND:event_jList2ValueChanged
    AddressCard card = (AddressCard) jList2.getSelectedValue();
    if (card!=null)
    {
      jxTreeTable.setTreeTableModel(new AddressCardTreeTableModel(card));
      jxTreeTable.setShowsRootHandles(true);
      jxTreeTable.expandAll();
      //jEditorPane1.setText(card.toLongString());
    }
  }//GEN-LAST:event_jList2ValueChanged
  
  /**
   * @param args the command line arguments
   */
  public static void main(String args[]) throws Exception
  {
    UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    java.awt.EventQueue.invokeLater(new Runnable()
    {
      public void run()
      {
        new Addressbook().setVisible(true);
      }
    });
  }
  
  private ListModel getAddressBooks()
  {
    return new AbstractListModel()
    {
      /**
       * Returns the value at the specified index.
       * @param index the requested index
       * @return the value at <code>index</code>
       */
      public Object getElementAt(int index)
      {
        return AddressBookManager.getAllAddressBooks().get(index);
      }
      
      /**
       *
       * Returns the length of the list.
       * @return the length of the list
       */
      public int getSize()
      {
         int n = AddressBookManager.getAllAddressBooks().size();
         return n;
      }
      
    };
  }
  
  protected JXTreeTable jxTreeTable;
    // Variables declaration - do not modify//GEN-BEGIN:variables
    protected javax.swing.JButton jButton1;
    protected javax.swing.JButton jButton2;
    protected javax.swing.JButton jButton3;
    protected javax.swing.JButton jButton4;
    protected javax.swing.JButton jButton5;
    protected javax.swing.JButton jButton6;
    protected javax.swing.JButton jButton7;
    protected javax.swing.JLabel jLabel1;
    protected javax.swing.JList jList1;
    protected javax.swing.JList jList2;
    protected javax.swing.JMenu jMenu1;
    protected javax.swing.JMenuBar jMenuBar1;
    protected javax.swing.JPanel jPanel1;
    protected javax.swing.JPanel jPanel2;
    protected javax.swing.JPanel jPanel3;
    protected javax.swing.JScrollPane jScrollPane1;
    protected javax.swing.JScrollPane jScrollPane2;
    protected javax.swing.JScrollPane jScrollPane3;
    protected javax.swing.JSplitPane jSplitPane1;
    protected javax.swing.JSplitPane jSplitPane2;
    protected javax.swing.JTextField jTextField1;
    protected javax.swing.JToolBar jToolBar1;
    // End of variables declaration//GEN-END:variables
  
}
