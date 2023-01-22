#include "ide.h"

bool ReFormatJSON_XML(String& text, bool xml)
{
	if(xml) {
		try {
			XmlNode n = ParseXML(text);
			text = AsXML(n);
		}
		catch(XmlError) {
			Exclamation("Error passing the XML!");
			return false;
		}
	}
	else {
		Value v = ParseJSON(text);
		if(v.IsError()) {
			Exclamation("Error passing the JSON!");
			return false;
		}
		text = AsJSON(v, true);
	}
	return true;
}

void Ide::FormatJSON_XML(bool xml)
{
	int l, h;
	bool sel = editor.GetSelection(l, h);
	if((sel ? h - l : editor.GetLength()) > 75 * 1024 * 1024) {
		Exclamation("Too big to reformat");
		return;
	}
	String text;
	if(sel)
		text = editor.GetSelection();
	else {
		SaveFile();
		text = LoadFile(editfile);
	}
	if(!ReFormatJSON_XML(text, xml))
		return;
	editor.NextUndo();
	if(sel) {
		editor.Remove(l, h - l);
		editor.SetSelection(l, l + editor.Insert(l, text));
	}
	else {
		editor.Remove(0, editor.GetLength());
		editor.Insert(0, text);
	}
}

void Ide::FormatJSON() { FormatJSON_XML(false); }
void Ide::FormatXML() { FormatJSON_XML(true); }

void ReformatCpp(CodeEditor& editor, const String& clang_format_path, bool setcursor)
{
	int64 l, h;
	bool sel = editor.GetSelection(l, h);

	String cmd = "clang-format ";
	if(sel) {
		l = editor.GetLine(l) + 1;
		h = editor.GetLine(h) + 1;
		cmd << "--lines=" << l << ":" << h << " ";
	}

	String temp_path = CacheFile(AsString(Random()) + AsString(Random()) + "_to_format.cpp");
	{
		FileOut out(temp_path);
		editor.Save(out, CHARSET_UTF8, TextCtrl::LE_LF);
		if(out.IsError()) {
			Exclamation("Failed to save temporary file \1" + temp_path);
			return;
		}
	}

	cmd << "\"--style=file:" << clang_format_path << "\" ";
	
	String reformatted = Sys(cmd + temp_path);
	
	DeleteFile(temp_path);
	
	if(reformatted.IsVoid()) {
		Exclamation("clang-format has failed. Is it installed?");
		return;
	}

	Vector<String> ln = Split(reformatted, '\n', false);
	for(String& s : ln)
		s.TrimEnd("\r");
	int n = editor.GetLineCount();
	l = h = n;
	for(int i = 0; i < n; i++)
		if(i >= ln.GetCount() || editor.GetUtf8Line(i) != ln[i]) {
			l = i;
			break;
		}
	for(int i = 0; i < n; i++)
		if(i >= ln.GetCount() || editor.GetUtf8Line(n - 1 - i) != ln[ln.GetCount() - 1 - i]) {
			h = i;
			break;
		}
	
	if(l + h >= n)
		return;
	
	editor.NextUndo();
	int from = editor.GetPos(l);
	editor.Remove(from, editor.GetPos(editor.GetLineCount() - h) - from);
	ln.Remove(0, l);
	ln.Trim(ln.GetCount() - h);
	if(setcursor)
		editor.SetCursor(editor.Insert(from, Join(ln, "\n") + "\n", CHARSET_UTF8));
}

void Ide::ReformatFile()
{
	String clang_format_path;

	for(String dir = GetFileFolder(editfile); dir.GetCount() > 3 && IsNull(clang_format_path);
	    dir = GetFileFolder(dir)) {
		for(String fn : { ".clang-format", "_clang-format" }) {
			String p = AppendFileName(dir, fn);
			if(FileExists(p)) {
				clang_format_path = p;
				break;
			}
		}
	}

	if(IsNull(clang_format_path)) {
		Exclamation("Missing .clang-format.");
		return;
	}

	ReformatCpp(editor, clang_format_path, true);
}

void Ide::ReformatComment() { editor.ReformatComment(); }
