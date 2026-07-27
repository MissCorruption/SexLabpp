; CI-only stub for Papyrus compile checks.
; Not for shipping — real VRIK sources are Nexus-only.
ScriptName VRIK Hidden

int Function VrikGetBuildNumber() global
	return 0
EndFunction

float Function VrikGetSetting(string asSetting) global
	return 0.0
EndFunction

Function VrikSetSetting(string asSetting, float afValue) global
EndFunction

Function VrikSetGesture(string asGesture, int aiValue) global
EndFunction

Function VrikRestoreSettings() global
EndFunction

Function VrikSetProfileAction(int aiGesture, string asEventName) global
EndFunction

Function VrikBeginGestureProfile() global
EndFunction

Function VrikEndGestureProfile() global
EndFunction
