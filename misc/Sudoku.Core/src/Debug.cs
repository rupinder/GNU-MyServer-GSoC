using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Sudoku
{
    class Debug
    {
        static public void PrintVector(ref int[] vector, int nCol)
        {
            Console.WriteLine("//---------------------------------------------------------//");
            for (int i = 0; i < vector.GetLength(0); i++)
            {
                Console.Write(vector[i]);
                Console.Write(" ");
                if ((i+1)%nCol == 0)
                    Console.WriteLine(" ");
            }
            Console.WriteLine(" ");
            Console.WriteLine("//---------------------------------------------------------//");
        }
        static public void PrintArr(ref int[,] tbl)
        {
            Console.WriteLine("//---------------------------------------------------------//");
            for (int i = 0; i < tbl.GetLength(1); i++)//row
            {
                for (int j = 0; j < tbl.GetLength(0); j++)//col
                {
                    Console.Write(tbl[j, i]);
                    Console.Write(" ");
                }
                Console.WriteLine(" ");
            }
            Console.WriteLine("//---------------------------------------------------------//");
        }
        static public void PrintExArr(ref int[,] tbl)
        {
            Console.WriteLine("//---------------------------------------------------------//");
            for (int i = 0; i < tbl.GetLength(1); i++)
            {
                Console.Write("Line:");
                Console.Write(tbl[0, i]);
                Console.Write("\t");
                for (int j = 1; j < tbl.GetLength(0); j++)
                {
                    Console.Write(tbl[j, i]);
                    Console.Write(" ");
                }
                Console.WriteLine(" ");
            }
            Console.WriteLine("//---------------------------------------------------------//");
        }
    }
}
