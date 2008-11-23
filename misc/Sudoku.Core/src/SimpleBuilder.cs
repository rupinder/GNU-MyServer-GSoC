using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Sudoku
{
    public class SimpleBuilder
    {
        public SimpleBuilder(int[,] input, int rows, int columns)
        {
            cover = null;
            nBoxRows = rows;
            nBoxColumns = columns;
        }

        public bool Run(int[,] input, int[] symbols)
        {
            nCellTaken = input.GetLength(0) * input.GetLength(1);
            nRowCond = input.GetLength(1) * symbols.GetLength(0);
            nColumnCond = input.GetLength(0) * symbols.GetLength(0);
            nBoxCond = input.GetLength(0) * input.GetLength(1);//cells nr/minDim*minDim

            nCoverColumns = nCellTaken + nRowCond + nColumnCond + nBoxCond;
            nCoverRows = input.GetLength(0) * input.GetLength(1) * symbols.GetLength(0);
            cover = new int[nCoverColumns * nCoverRows];

            if (!AddCells(input, symbols) || !AddRows(input, symbols) || !AddCols(input, symbols) || !AddBoxes(input, symbols, nBoxRows, nBoxColumns))
                return false;
            //Debug.PrintVector(ref cover, nCoverColumns);
            return true;
        }

        protected bool AddCells(int[,] input, int[] symbols)
        {
            int nCrtRow = 0, nCrtCol = 0;
            for (int j = 0; j < input.GetLength(1); j++)//rows
            {
                for (int i = 0; i < input.GetLength(0); i++)//columns
                {
                    for (int k = 0; k < symbols.GetLength(0); k++)//symbols
                    {
                        if (input[i, j] == symbols[k])
                        {
                            if (1 == cover[nCrtRow * nCoverColumns + nCrtCol])
                                return false;
                            else
                                cover[nCrtRow * nCoverColumns + nCrtCol] = 1;
                        }
                        else
                            cover[nCrtRow * nCoverColumns + nCrtCol] = 0;
                        nCrtRow++;
                    }
                    nCrtCol++;
                }
            }
            //Debug.PrintVector(ref cover, nCoverColumns);
            return true;
        }
        protected bool AddRows(int[,] input, int[] symbols)
        {
            int nCrtRow = 0, nCrtCol = 0;
            for (int j = 0; j < input.GetLength(1); j++)//rows
            {
                for (int i = 0; i < input.GetLength(0); i++)//columns
                {
                    for (int k = 0; k < symbols.GetLength(0); k++)//symbols
                    {
                        if (input[i, j] == symbols[k])
                        {
                            if (1 == cover[nCrtRow * nCoverColumns + (nCellTaken + nCrtCol + k)])
                                return false;
                            else
                                cover[nCrtRow * nCoverColumns + (nCellTaken + nCrtCol + k)] = 1;
                        }
                        else
                            cover[nCrtRow * nCoverColumns + (nCellTaken + nCrtCol + k)] = 0;
                        nCrtRow++;
                    }
                }
                nCrtCol += symbols.GetLength(0);
            }
            //Debug.PrintVector(ref cover, nCoverColumns);
            return true;
        }
        protected bool AddCols(int[,] input, int[] symbols)
        {
            int nCrtRow = 0, nCrtCol = 0;
            for (int j = 0; j < input.GetLength(1); j++)//rows
            {
                for (int i = 0; i < input.GetLength(0); i++)//columns
                {
                    for (int k = 0; k < symbols.GetLength(0); k++)//symbols
                    {
                        if (input[i, j] == symbols[k])
                        {
                            if (1 == cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nCrtCol + k)])
                                return false;
                            else
                                cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nCrtCol + k)] = 1;
                        }
                        else
                            cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nCrtCol + k)] = 0;
                        nCrtRow++;
                    }
                    nCrtCol += symbols.GetLength(0);
                }
                nCrtCol = 0;
            }
            //Debug.PrintVector(ref cover, nCoverColumns);
            return true;
        }
        protected bool AddBoxes(int[,] input, int[] symbols, int rows, int columns)
        {
            int nCrtRow = 0, nCrtCol = 0, nCrtBox = 0;
            for (int i = 0; i < input.GetLength(0); i++)//columns
            {
                for (int j = 0; j < input.GetLength(1); j++)//rows
                {
                    nCrtBox = i / columns + j / rows * input.GetLength(0) / columns;
                    nCrtCol = nCrtBox * symbols.GetLength(0);
                    for (int k = 0; k < symbols.GetLength(0); k++)//symbols
                    {
                        if (input[i, j] == symbols[k])
                        {
                            if (1 == cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nColumnCond + nCrtCol + k)])
                                return false;
                            else
                                cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nColumnCond + nCrtCol + k)] = 1;
                        }
                        else
                            cover[nCrtRow * nCoverColumns + (nCellTaken + nRowCond + nColumnCond + nCrtCol + k)] = 0;
                        nCrtRow++;
                    }
                }
            }
            //Debug.PrintVector(ref cover, nCoverColumns);
            return true;
        }
        public int[,] GetCover()
        {
            int[,] ret = new int[nCoverColumns, nCoverRows];
            for (int i = 0; i < cover.GetLength(0); i++)
                ret[i % nCoverColumns, i / nCoverColumns] = cover[i];

            //Debug.PrintVector(ref cover, nCoverColumns);
            //Debug.PrintArr(ref ret);
            return ret;
        }

        // exact cover
        public int[] cover;
        int nCoverRows, nCoverColumns;
        int nCellTaken, nRowCond, nColumnCond, nBoxCond;
        int nBoxRows, nBoxColumns;
    }
}
