using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;

namespace Sudoku
{
    public class Configuration
    {
        public Configuration()
        {
	  rows = 0;
	  columns = 0;
	  boxRows = 0;
	  boxColumns = 0;
	}

        public Configuration(string sFilename)
        {
            m_sFileName = sFilename;
            //ReadCfg(sFilename);
        }
        public bool ReadCfg()
        {
            return ReadCfg(m_sFileName);
        }
        public bool ReadCfg(string sFilename)
        {
            try
            {
                bool bSkipRows = true;
                string sCrtTableName="";
                int nCrtRow = 0;
		XmlReader cfgReader = XmlReader.Create(m_sFileName);
		//XmlTextReader cfReader = new XmlTextReader(m_sFileName);
                while (cfgReader.Read())
                {
                    if (cfgReader.NodeType != XmlNodeType.Element )
                        continue;
                    switch (cfgReader.Name)
                    {
                        case "CurrentTable":
                            cfgReader.MoveToFirstAttribute();
                            cfgReader.ReadAttributeValue();
                            sCrtTableName = cfgReader.Value;
                            bSkipRows = false;
                            break;
                        // handle Table nodes
                        case "Table":
                            nCrtRow = 0;
                            cfgReader.MoveToFirstAttribute();
                            cfgReader.ReadAttributeValue();
                            if (sCrtTableName.CompareTo("") != 0 && sCrtTableName.CompareTo(cfgReader.Value) != 0)
                            {
                                bSkipRows = true;
                                continue;
                            }
                            if (sudokuTable != null)
                                return false;
                            cfgReader.MoveToNextAttribute();
                            cfgReader.ReadAttributeValue();
                            rows = Convert.ToInt32(cfgReader.Value);
                            cfgReader.MoveToNextAttribute();
                            cfgReader.ReadAttributeValue();
                            columns = Convert.ToInt32(cfgReader.Value);
                            cfgReader.MoveToNextAttribute();
                            cfgReader.ReadAttributeValue();
                            boxRows = Convert.ToInt32(cfgReader.Value);
                            cfgReader.MoveToNextAttribute();
                            cfgReader.ReadAttributeValue();
                            boxColumns = Convert.ToInt32(cfgReader.Value);
                            if (sudokuTable != null)
                            {
                                Console.WriteLine("Already have a table");
                                return false;
                            }
                            sudokuTable = new int[columns, rows];
                            bSkipRows = false;
                            break;
                        // handle Row nodes
                        case "Row":
                            if (bSkipRows)
                                continue;
                            string[] arrNumbers = cfgReader.ReadString().Split(',');
                            for (int j = 0; j < arrNumbers.Length; j++)
                            {
                                if ( sudokuTable == null )
                                    break;
                                sudokuTable[j,nCrtRow] = Convert.ToInt32(arrNumbers[j]);
                            }
                            nCrtRow++;
                            break;
                    }
                }
            }
            catch (Exception pEx)
            {
                Console.WriteLine(pEx.Message);
                return false;
            }
            return true;
        }
        protected String m_sFileName;
        public int [,]sudokuTable;
        public int rows, columns, boxRows, boxColumns;
    }
}
