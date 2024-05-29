/*
  ==============================================================================

    FileSelectorComponent.h
    Created: 19 Aug 2023 4:01:49pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/*** TODO:
 -limit recent files history length
 */

class FileSelectorComponent	:	public juce::FilenameComponent
{
public:
	FileSelectorComponent(const juce::File &currentFile, const juce::String &fileBrowserWildcard, const juce::String &enforcedSuffix, const juce::String &textWhenNothingSelected, bool allowDirectory = false)
	:	FilenameComponent("FileComp", currentFile,
						  false,	// can edit filename
						  allowDirectory,		// isDirectory
						  false,	// isForSaving
						  fileBrowserWildcard, enforcedSuffix, textWhenNothingSelected)
	{}
	~FileSelectorComponent(){}
	
	void getRecentFilesFromUserApplicationDataDirectory(){
		//	manage directory
		juce::File appDataDirectory = juce::File::getSpecialLocation
			(juce::File::SpecialLocationType::userApplicationDataDirectory)
			.getChildFile("nvssynthesis")
			.getChildFile("slicer_granular");
		appDataDirectory.createDirectory();	// only creates if nonexistent
		
		recentFilesListFile = appDataDirectory.getChildFile("Recent Audio Files.txt");
		recentFilesListFile.create();		// only creates if nonexistent
		recentFilesListFile.readLines(recentFiles);
		
		cleanRecentFiles();
		
		setRecentlyUsedFilenames(recentFiles);
	}
	
	void cleanRecentFiles(){
		recentFiles.removeEmptyStrings();

		std::vector<int> badFileIndices(0);
		for (int i = 0; i < recentFiles.size(); ++i){
			juce::String fn = recentFiles[i];
			if (juce::File::isAbsolutePath(fn)){
				juce::File tmpFile = juce::File( fn );
				if (! tmpFile.existsAsFile()){
					badFileIndices.push_back(i);
				}
			}
			else {
				badFileIndices.push_back(i);
			}
		}
		// reverse ordered sort to remove the rightmost first, so it doesn't mess up order of those not yet removed
		std::sort(badFileIndices.rbegin(), badFileIndices.rend());
		for (auto idx : badFileIndices){
			recentFiles.remove(idx);
		}
	}
	
	void pushRecentFilesToFile(){
		juce::StringArray thisSessionsRecentFiles = getRecentlyUsedFilenames();
		for (auto &f : thisSessionsRecentFiles){
			recentFiles.add(f);
		}
		recentFiles.removeDuplicates(false);
		recentFilesListFile.deleteFile();
		recentFilesListFile.create();
		for (auto &f : thisSessionsRecentFiles){
			recentFilesListFile.appendText(f);
			if (f != *(thisSessionsRecentFiles.end() - 1)){
				recentFilesListFile.appendText(juce::newLine);
			}
		}
	}
	
private:
	juce::StringArray recentFiles;
	juce::File recentFilesListFile;
};
