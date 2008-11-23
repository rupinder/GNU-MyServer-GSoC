using System;
using System.Collections.Generic;

namespace Sudoku
{
    public static class Program
    {
      /*
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        static void Main()
        {
            // load sudoku table
            //Configuration cfg = new Configuration("cfg.xml");
	    Configuration cfg = new Configuration("./cfg.xml");
            if (!cfg.ReadCfg())
                return;
	    if ( Run(cfg.sudokuTable) )
                Console.WriteLine("Congratulations you solved this Sudoku!");
            else
                Console.WriteLine("Your table has errors! Try again.");
        }
      */
      public static bool Run(ref Configuration cfg)
      {
            // check if table ok
            SimpleBuilder bld = new SimpleBuilder(cfg.sudokuTable, cfg.boxRows, cfg.boxColumns);
            //Debug.PrintArr(ref cfg.sudokuTable);
            int[] symbols = new int[cfg.sudokuTable.GetLength(0)];
            for (int i = 0; i < symbols.GetLength(0); i++)
                symbols[i] = i + 1;
            if (!bld.Run(cfg.sudokuTable, symbols))
                return false;

            // sudoku solver
            int[,] locCover = bld.GetCover();
            //Debug.PrintArr(ref locCover);
            if (locCover == null)
                return false;

            SimpleSolver slv = new SimpleSolver(locCover);
            ///////////////////////
            // build resolver table
            int[,] locTable = new int[locCover.GetLength(0) + 1, locCover.GetLength(1) + 1];
            for (int i = 0; i < locCover.GetLength(0); i++)
            {
                for (int j = 0; j < locCover.GetLength(1); j++)
                {
                    locTable[i + 1, j + 1] = locCover[i, j];
                    locTable[0, j] = j;
                }
                locTable[i, 0] = i;
            }
            locTable[0, locCover.GetLength(1)] = locCover.GetLength(1);
            locTable[locCover.GetLength(0), 0] = locCover.GetLength(0);
            ///////////////////////

            List<int> arrSolution = new List<int>(locCover.GetLength(1));
            if ( !slv.CheckConflicts(locTable) )
	      //    Console.WriteLine("Givens Sudoku table valid(should have a solution)");
	      //else
            {
                //Debug.PrintArr(ref locTable);
                //Console.WriteLine("Wrong givens!");
                return false;
            }
            if ( slv.Resolve(locTable, ref arrSolution) )
	      {
                //Console.WriteLine("Congratulations you solved this Sudoku!");
		return true;
	      }
            else
	      {
                //Console.WriteLine("Your table has errors! Try again.");
		return false;
	      }
      }
    }
}
