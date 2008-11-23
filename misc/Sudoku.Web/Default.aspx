<%@ Page Language="C#" AutoEventWireup="true"  CodeFile="Default.aspx.cs" Inherits="SudokuGUI" StyleSheetTheme="Default" %>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <title>Sudoku game</title>
</head>
<body>
<form id="form1" runat="server">
<table align="center" style="width: auto;">
    <tr>
        <td>
            <asp:TextBox ID="tb00" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb10" runat="server" Font-Overline="False"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb20" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb30" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb40" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb50" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb60" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb70" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb80" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb01" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb11" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb21" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb31" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb41" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb51" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb61" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb71" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb81" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb02" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb12" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb22" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb32" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb42" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb52" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb62" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb72" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb82" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb03" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb13" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb23" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb33" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb43" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb53" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb63" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb73" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb83" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb04" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb14" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb24" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb34" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb44" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb54" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb64" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb74" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb84" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb05" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb15" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb25" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb35" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb45" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb55" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb65" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb75" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb85" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb06" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb16" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb26" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb36" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb46" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb56" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb66" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb76" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb86" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb07" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb17" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb27" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb37" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb47" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb57" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb67" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb77" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb87" runat="server"></asp:TextBox>
        </td>
    </tr>
    <tr>
        <td>
            <asp:TextBox ID="tb08" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb18" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb28" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb38" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb48" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb58" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb68" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb78" runat="server"></asp:TextBox>
        </td>
        <td>
            <asp:TextBox ID="tb88" runat="server"></asp:TextBox>
        </td>
    </tr>
</table>
<asp:Button ID="btnCheckTable" runat="server" BackColor="#339933" 
    onclick="CheckTable_Click" Text="Check Values" />
Result: <asp:TextBox ID="txtResult" runat="server"/>
</form>
</body>
</html>
