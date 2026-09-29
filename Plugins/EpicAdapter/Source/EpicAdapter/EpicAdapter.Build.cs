// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;  // Required for Path class
using System.Linq;  // Required for LINQ methods like Take()

public class EpicAdapter : ModuleRules
{
	public EpicAdapter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Enable early initialization support
		bRequiresImplementModule = false;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",              // Required for UUserWidget and UI components
				"Slate",            // Required for Slate UI framework
				"SlateCore",        // Required for Slate core functionality
				"EnhancedInput",    // Required for Enhanced Input system
				"ApplicationCore",  // Required for early initialization
				// ... add other public dependencies that you statically link with here ...
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",         // Required for IPluginManager
				"InputCore",        // Required for basic input handling
				"GameplayTags",     // Often used with Enhanced Input
				"DeveloperSettings", // For project settings integration
				"Json",             // For JSON DOM types (FJsonObject, FJsonValue, FJsonSerializer)
				"JsonUtilities",    // For convenience JSON helpers if needed
				// ... add private dependencies that you statically link with here 	
			}
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// EOS SDK Integration - use Target.ProjectFile to get project root reliably
		string ProjectRoot = Path.GetDirectoryName(Target.ProjectFile.FullName);
		string EosSdkPath = Path.Combine(ProjectRoot, "ThirdParty/EOS-SDK-47370208-Release-v1.18.1.2/SDK");
		string ResolvedEosSdkPath = Path.GetFullPath(EosSdkPath);
		
		System.Console.WriteLine("=== EpicAdapter Build Configuration ===");
		System.Console.WriteLine($"Module Directory: {ModuleDirectory}");
		System.Console.WriteLine($"Target EOS SDK Path: {EosSdkPath}");
		System.Console.WriteLine($"Resolved EOS SDK Path: {ResolvedEosSdkPath}");
		System.Console.WriteLine($"Target Platform: {Target.Platform}");
		System.Console.WriteLine($"Target Configuration: {Target.Configuration}");
		
		// Check if the resolved path exists
		if (Directory.Exists(ResolvedEosSdkPath))
		{
			System.Console.WriteLine($"✓ EOS SDK directory found at: {ResolvedEosSdkPath}");
			
			string IncludePath = Path.Combine(ResolvedEosSdkPath, "Include");
			if (Directory.Exists(IncludePath))
			{
				PublicIncludePaths.Add(IncludePath);
				System.Console.WriteLine($"✓ EOS SDK include path added: {IncludePath}");
				
				// List header files for verification
				try
				{
					string[] headerFiles = Directory.GetFiles(IncludePath, "*.h", SearchOption.AllDirectories);
					System.Console.WriteLine($"✓ Found {headerFiles.Length} header files in EOS SDK");
					if (headerFiles.Length > 0)
					{
						System.Console.WriteLine($"  Sample headers: {string.Join(", ", headerFiles.Take(3).Select(Path.GetFileName))}");
					}
				}
				catch (System.Exception ex)
				{
					System.Console.WriteLine($"⚠ Warning: Could not enumerate header files: {ex.Message}");
				}
			}
			else
			{
				System.Console.WriteLine($"✗ EOS SDK include directory not found: {IncludePath}");
			}
			
			// Platform-specific library linking
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string LibPath = Path.Combine(ResolvedEosSdkPath, "Lib");
				string EosSdkLibPath = Path.Combine(LibPath, "EOSSDK-Win64-Shipping.lib");
				
				System.Console.WriteLine($"Looking for Win64 library at: {EosSdkLibPath}");
				
				if (File.Exists(EosSdkLibPath))
				{
					PublicAdditionalLibraries.Add(EosSdkLibPath);
					
					// Get file info for detailed logging
					var fileInfo = new FileInfo(EosSdkLibPath);
					System.Console.WriteLine($"✓ EOS SDK library linked: {EosSdkLibPath}");
					System.Console.WriteLine($"  Library size: {fileInfo.Length / (1024 * 1024)} MB");
					System.Console.WriteLine($"  Last modified: {fileInfo.LastWriteTime}");
				}
				else
				{
					System.Console.WriteLine($"✗ EOS SDK library not found: {EosSdkLibPath}");
					
					// Check what files are actually in the lib directory
					if (Directory.Exists(LibPath))
					{
						try
						{
							string[] libFiles = Directory.GetFiles(LibPath, "*.lib");
							System.Console.WriteLine($"Available .lib files in {LibPath}:");
							foreach (string libFile in libFiles)
							{
								System.Console.WriteLine($"  - {Path.GetFileName(libFile)}");
							}
						}
						catch (System.Exception ex)
						{
							System.Console.WriteLine($"⚠ Could not list lib directory: {ex.Message}");
						}
					}
					else
					{
						System.Console.WriteLine($"✗ Lib directory does not exist: {LibPath}");
					}
				}
			}
			else
			{
				System.Console.WriteLine($"⚠ Platform {Target.Platform} not explicitly configured for EOS SDK linking");
			}
			
			PublicDefinitions.Add("WITH_EOS_SDK=1");
			System.Console.WriteLine("✓ EOS SDK integration ENABLED (WITH_EOS_SDK=1)");
		}
		else
		{
			// SDK not found, disable EOS integration
			System.Console.WriteLine($"✗ EOS SDK directory not found at: {ResolvedEosSdkPath}");
			
			// Check if parent directories exist to help with troubleshooting
			string parentPath = ResolvedEosSdkPath;
			int levelsUp = 0;
			while (!Directory.Exists(parentPath) && levelsUp < 5)
			{
				parentPath = Path.GetDirectoryName(parentPath);
				levelsUp++;
				if (!string.IsNullOrEmpty(parentPath))
				{
					System.Console.WriteLine($"  Checking parent {levelsUp} level(s) up: {parentPath} - {(Directory.Exists(parentPath) ? "EXISTS" : "NOT FOUND")}");
				}
			}
			
			PublicDefinitions.Add("WITH_EOS_SDK=0");
			System.Console.WriteLine("✗ EOS SDK integration DISABLED (WITH_EOS_SDK=0)");
		}
		
		System.Console.WriteLine("=== End EpicAdapter Build Configuration ===");
	}
}