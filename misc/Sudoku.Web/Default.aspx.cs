using System;
using System.Configuration;
using System.Data;
using System.Web;
using System.Web.Security;
using System.Web.UI;
using System.Web.UI.HtmlControls;
using System.Web.UI.WebControls;
using System.Web.UI.WebControls.WebParts;
using Sudoku;

public partial class SudokuGUI : System.Web.UI.Page
{
    protected void Page_Load(object sender, EventArgs e)
    {
        Setup_Fields();
    }
    protected void CheckTable_Click(object sender, EventArgs e)
    {
        TextBox current = new TextBox();
        for (int i = 0; i < 9; i++)
        {
            for (int j = 0; j < 9; j++)
            {
                current = (TextBox)form1.FindControl("tb" + i.ToString() + j.ToString());
		if ( i < m_cfg.columns && j < m_cfg.rows )
		  m_cfg.sudokuTable[i,j] = Convert.ToInt32(current.Text);
            }
        }
	current = (TextBox)form1.FindControl("txtResult");
	if ( Sudoku.Program.Run(ref m_cfg) )
	  current.Text = "Table values represent a solution!";
	else
	  current.Text = "Table has errors!";
    }

    /// <summary>
    /// This is the place where correct properties are set for the fields.
    /// </summary>
    protected void Setup_Fields()
    {
	m_cfg = new Sudoku.Configuration("./App_Data/cfg.xml");
	bool bCfgOK = m_cfg.ReadCfg();

        TextBox current = new TextBox();
        for (int i = 0; i < 9; i++)
        {
            for (int j = 0; j < 9; j++)
            {
                current = (TextBox)form1.FindControl("tb" + i.ToString() + j.ToString());
                current.Columns = 3;
                current.Height = 20;
		if ( bCfgOK && i < m_cfg.columns && j < m_cfg.rows )
		  {
		    current.Text = m_cfg.sudokuTable[i,j].ToString();
		  }
            }
        }
	current = (TextBox)form1.FindControl("txtResult");
	current.Text = "";
    }
    protected Sudoku.Configuration m_cfg;
}
