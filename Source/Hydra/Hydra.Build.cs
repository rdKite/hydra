using System.IO;
using UnrealBuildTool;

public class Hydra : ModuleRules
{
	public Hydra(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"Json",
			"JsonUtilities",
			"NetCore"
		});

		PublicIncludePaths.Add(ModuleDirectory);

		// data/*.json is the single source of gameplay data. A dev checkout reads data/ directly
		// (edits only need Hydra.ReloadValues); the copy in Config/Hydra/ is what a packaged
		// build uses. Config/Hydra/ is generated and ignored by git.
		string ProjectDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
		string DataDir = Path.Combine(ProjectDir, "data");
		string ConfigDir = Path.Combine(ProjectDir, "Config", "Hydra");
		if (Directory.Exists(DataDir))
		{
			Directory.CreateDirectory(ConfigDir);
			foreach (string Source in Directory.GetFiles(DataDir, "*.json"))
			{
				File.Copy(Source, Path.Combine(ConfigDir, Path.GetFileName(Source)), true);
			}
		}
		RuntimeDependencies.Add("$(ProjectDir)/Config/Hydra/...", StagedFileType.UFS);
	}
}
