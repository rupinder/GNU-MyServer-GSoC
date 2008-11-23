using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Sudoku
{
    public class SimpleSolver
    {
        public SimpleSolver(int[,] inputTable)
        {
        }
        // inputTable first row/column contain row/column number
        public bool Resolve(int[,] input, ref List<int> output)
        {
            if ( input.GetLength(0) == 1 )
                return true;// empty array
            //Debug.DebugPrintArr(ref input);

            /////////////////////////////////////
            // find c
            int nOnesMin = input.GetLength(0), nColumn = 0, nRow = 0, nSolutionRow = 0;
            for (int i = input.GetUpperBound(0); i > 0; i--)
            {
                int nCrtOnes = 0;
                for (int j = input.GetUpperBound(1); j > 0; j--)
                {
                    if ( 1 == input[i,j] )
                        nCrtOnes++;
                }
                if (nOnesMin > nCrtOnes && nCrtOnes > 0)
                {
                    nColumn = i;
                    nOnesMin = nCrtOnes;
                }
            }
            /////////////////////////////////////

            for (int i = input.GetUpperBound(1); i > 0; i--)
            {
                if (input[nColumn, i] != 0)
                {
                    nSolutionRow = input[0, i];
                    nRow = i;
                    break;
                }
            }
            output.Add(nSolutionRow);// save row

            List<int> delColumns = new List<int>(), delRows = new List<int>();
            // delete all columns form row
            for (int i = input.GetUpperBound(0); i > 0; i--)
            {
                if (1 != input[i, nRow])
                    continue;

                delColumns.Add(i);
                for (int j = input.GetUpperBound(1); j > 0; j--)
                {
                    if ((1 == input[i, j] || 1 == input[nColumn, j]) && !delRows.Contains(j))
                        delRows.Add(j);
                }
            }
            if (delColumns.Count == 0 && delRows.Count == 0)
                return true;
            int[,] reduced = new int[input.GetLength(0) - delColumns.Count, input.GetLength(1) - delRows.Count];
            int nCrtColumn = 0, nCrtRow = 0;
            for (int i = 0; i < input.GetLength(0); i++)
            {
                if (delColumns.Contains(i))
                    continue;
                nCrtRow = 0;
                for (int j = 0; j < input.GetLength(1); j++)
                {
                    if (delColumns.Contains(i) || delRows.Contains(j))
                        continue;

                    reduced[nCrtColumn, nCrtRow++] = input[i, j];
                }
                nCrtColumn++;
            }

            return Resolve(reduced, ref output);
        }
        public bool CheckConflicts(int[,] table)
        {
            //Debug.PrintArr(ref table);
            int nOnes;
            bool bRet = true;
            List<int> pb = new List<int>(table.GetLength(0));
            for (int i = 1; i < table.GetLength(0); i++)
            {
                nOnes = 0;
                for (int j = 1; j < table.GetLength(1); j++)
                {
                    if (table[i, j] != 0)
                        nOnes++;
                }
                if (nOnes > 1)
                {
                    bRet = false;
                    pb.Add(table[i, 0]);
                }
            }
            return bRet;
        }
    }
}
