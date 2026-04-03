using System;
using System.Diagnostics;
using System.IO;

namespace CasperManager
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("=================================================");
            Console.WriteLine("  Casper AI: Sovereign Training Manager (C#)     ");
            Console.WriteLine("=================================================");
            Console.WriteLine("[Manager] Starting sovereign training sequence...");

            string projectRoot = FindProjectRoot();
            string perlScript = Path.Combine(projectRoot, "data_prep.pl");
            string trainerExe = Path.Combine(projectRoot, @"Core_CPP\trainer.exe");
            string dataFile = Path.Combine(projectRoot, @"Data_Training\sovereign_knowledge.txt");

            // Step 1: Run Perl Data Preprocessor
            Console.WriteLine("\n[Step 1] Running Perl Knowledge Generator...");
            if (File.Exists(perlScript))
            {
                RunProcess("perl", perlScript);
            }
            else
            {
                Console.WriteLine("[Manager] data_prep.pl not found, skipping data generation.");
            }

            // Step 2: Ensure C++ Trainer is compiled
            // (Assuming user has g++ installed)
            Console.WriteLine("\n[Step 2] Building trainer via script...");
            RunProcess("powershell.exe", $"-ExecutionPolicy Bypass -File \"{Path.Combine(projectRoot, @"scripts\build_trainer.ps1")}\"");

            // Step 3: Run the Training Process
            if (File.Exists(trainerExe))
            {
                Console.WriteLine("\n[Step 3] Executing C++ Training Engine...");
                RunProcess(trainerExe, dataFile);
            }
            else
            {
                Console.WriteLine("[Error] Trainer compilation failed.");
            }

            Console.WriteLine("\n[Manager] Training sequence completed. Casper is now smarter.");
            Console.WriteLine("=================================================");
        }

        static void RunProcess(string fileName, string arguments)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo
            {
                FileName = fileName,
                Arguments = arguments,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using (Process process = Process.Start(startInfo)!)
            {
                process.OutputDataReceived += (sender, e) => { if (e.Data != null) Console.WriteLine(e.Data); };
                process.ErrorDataReceived += (sender, e) => { if (e.Data != null) Console.WriteLine("[Error] " + e.Data); };
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();
                process.WaitForExit();
                if (process.ExitCode != 0)
                {
                    Console.WriteLine($"[Manager] Process failed: {fileName} (exit {process.ExitCode})");
                }
            }
        }

        static string FindProjectRoot()
        {
            string dir = AppContext.BaseDirectory;
            for (int i = 0; i < 8; i++)
            {
                if (File.Exists(Path.Combine(dir, "README.md")) &&
                    Directory.Exists(Path.Combine(dir, "Core_CPP")))
                {
                    return dir;
                }

                DirectoryInfo? parent = Directory.GetParent(dir);
                if (parent == null) break;
                dir = parent.FullName;
            }
            return Directory.GetCurrentDirectory();
        }
    }
}
