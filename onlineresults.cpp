/************************************************************************
    MeOS - Orienteering Software
    Copyright (C) 2009-2017 Melin Software HB

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Melin Software HB - software@melin.nu - www.melin.nu
    Eksoppsv�gen 16, SE-75646 UPPSALA, Sweden

************************************************************************/

#include "stdafx.h"

#include "resource.h"

#include <commctrl.h>
#include <commdlg.h>
#include <sys/stat.h>

#include "oEvent.h"
#include "gdioutput.h"

#include "onlineresults.h"
#include "meos_util.h"
#include <shellapi.h>

#include "gdiconstants.h"
#include "infoserver.h"
#include "meosException.h"
#include "Download.h"
#include "xmlparser.h"
#include "progress.h"

int AutomaticCB(gdioutput *gdi, int type, void *data);

static int OnlineCB(gdioutput *gdi, int type, void *data) {
  switch (type) {
    case GUI_BUTTON: {
      //Make a copy
      ButtonInfo bu=*static_cast<ButtonInfo *>(data);
      OnlineResults &ores = dynamic_cast<OnlineResults &>(*AutoMachine::getMachine(bu.getExtraInt()));

      return ores.processButton(*gdi, bu);
    }
    case GUI_LISTBOX:{
    }
  }
  return 0;
}

OnlineResults::~OnlineResults() {
  if (infoServer)
    delete infoServer;
}

int OnlineResults::processButton(gdioutput &gdi, ButtonInfo &bi) {

  if (bi.id == "ToURL")
    enableURL(gdi, gdi.isChecked(bi.id));
  else if (bi.id == "ToFile")
    enableFile(gdi, gdi.isChecked(bi.id));
  else if (bi.id == "BrowseFolder") {
    string res = gdi.getText("FolderName");
    res = gdi.browseForFolder(res, 0);
    if (!res.empty())
      gdi.setText("FolderName", res, true);
  }
  return 0;
}

void OnlineResults::settings(gdioutput &gdi, oEvent &oe, bool created) {
  int iv = interval;
  if (created) {
    iv = 10;
    url = oe.getPropertyString("MOPURL", "");
    file = oe.getPropertyString("MOPFolderName", "");
    oe.getAllClasses(classes);
  }

  string time;
  if (iv>0)
    time = itos(iv);

  settingsTitle(gdi, "Resultat online");
  startCancelInterval(gdi, "Save", created, IntervalSecond, time);

  int basex = gdi.getCX();
  gdi.pushY();
  gdi.fillRight();
  gdi.addListBox("Classes", 200,300,0, "Klasser:","", true);
  gdi.pushX();
  vector< pair<string, size_t> > d;
  gdi.addItem("Classes", oe.fillClasses(d, oEvent::extraNone, oEvent::filterNone));
  gdi.setSelection("Classes", classes);

  gdi.popX();

  gdi.popY();
  gdi.fillDown();


 // gdi.dropLine();
 // gdi.addInput("Interval", time, 10, 0, "Uppdateringsintervall (sekunder):");

  gdi.addSelection("Format", 200, 200, 0, "Exportformat:");
  gdi.addItem("Format", "MeOS Online Protocol XML", 1);
  gdi.addItem("Format", "IOF XML 2.0.3", 2);
  gdi.addItem("Format", "IOF XML 3.0", 3);
  gdi.selectItemByData("Format", dataType);

  gdi.addCheckbox("Zip", "Packa stora filer (zip)", 0, zipFile);
  int cx = gdi.getCX();
  gdi.fillRight();

  gdi.addCheckbox("ToURL", "Skicka till webben", OnlineCB, sendToURL).setExtra(getId());

  gdi.addString("", 0, "URL:");
  gdi.pushX();
  gdi.addInput("URL", url, 40, 0, "", "Till exempel X#http://www.results.org/online.php");
  gdi.dropLine(2.5);
  gdi.popX();
  gdi.addInput("CmpID", itos(cmpId), 10, 0, "T�vlingens ID-nummer:");
  gdi.addInput("Password", passwd, 15, 0, "L�senord:").setPassword(true);

  enableURL(gdi, sendToURL);

  gdi.setCX(cx);
  gdi.dropLine(5);
  gdi.fillRight();

  gdi.addCheckbox("ToFile", "Spara p� disk", OnlineCB, sendToFile).setExtra(getId());

  gdi.addString("", 0, "Mapp:");
  gdi.pushX();
  gdi.addInput("FolderName", file, 30);
  gdi.addButton("BrowseFolder", "Bl�ddra...", OnlineCB).setExtra(getId());
  gdi.dropLine(2.5);
  gdi.popX();

  gdi.addInput("Prefix", prefix, 10, 0, "Filnamnsprefix:");
  gdi.dropLine(2.8);
  gdi.popX();

  gdi.addInput("ExportScript", exportScript, 32, 0, "Skript att k�ra efter export:");
  gdi.dropLine(0.8);
  gdi.addButton("BrowseScript", "Bl�ddra...", AutomaticCB);

  gdi.setCY(gdi.getHeight());
  gdi.setCX(basex);

  gdi.fillDown();
  gdi.dropLine();
  gdi.addString("", fontMediumPlus, "Kontroller");
  RECT rc;
  rc.left = gdi.getCX();
  rc.right = gdi.getWidth();
  rc.top = gdi.getCY();
  rc.bottom = rc.top + 3;
  gdi.addRectangle(rc, colorDarkBlue, false);
  gdi.dropLine();
  vector<pControl> ctrl;
  oe.getControls(ctrl, true);

  vector< pair<pControl, int> > ctrlP;
  for (size_t k = 0; k< ctrl.size(); k++) {
    for (int i = 0; i < ctrl[k]->getNumberDuplicates(); i++) {
      ctrlP.push_back(make_pair(ctrl[k], oControl::getCourseControlIdFromIdIndex(ctrl[k]->getId(), i)));
    }
  }

  int width = gdi.scaleLength(130);
  int height = int(gdi.getLineHeight()*1.5);
  int xp = gdi.getCX();
  int yp = gdi.getCY();
  for (size_t k = 0; k< ctrlP.size(); k++) {
    string name = "#" + (ctrlP[k].first->hasName() ? ctrlP[k].first->getName() : ctrlP[k].first->getString());
    if (ctrlP[k].first->getNumberDuplicates() > 1)
      name += "-" + itos(oControl::getIdIndexFromCourseControlId(ctrlP[k].second).second + 1);
    gdi.addCheckbox(xp + (k % 6)*width, yp + (k / 6)*height, "C"+itos(ctrlP[k].second),
                    name, 0, ctrlP[k].first->isValidRadio());
  }
  gdi.dropLine();

  rc.top = gdi.getCY();
  rc.bottom = rc.top + 3;
  gdi.addRectangle(rc, colorDarkBlue, false);
  gdi.dropLine();

  formatError(gdi);
  if (errorLines.empty())
    gdi.addString("", 10, "help:onlineresult");

  enableFile(gdi, sendToFile);
}

void OnlineResults::enableURL(gdioutput &gdi, bool state) {
  gdi.setInputStatus("URL", state);
  gdi.setInputStatus("CmpID", state);
  gdi.setInputStatus("Password", state);
}

void OnlineResults::enableFile(gdioutput &gdi, bool state) {
  gdi.setInputStatus("FolderName", state);
  gdi.setInputStatus("BrowseFolder", state);
  gdi.setInputStatus("Prefix", state);
  gdi.setInputStatus("ExportScript", state);
  gdi.setInputStatus("BrowseScript", state);
}

void OnlineResults::save(oEvent &oe, gdioutput &gdi) {
  int iv=gdi.getTextNo("Interval");
  string folder=gdi.getText("FolderName");
  const string &xurl=gdi.getText("URL");

  if (!folder.empty())
    oe.setProperty("MOPFolderName", folder);

  if (!xurl.empty())
    oe.setProperty("MOPURL", xurl);

  sendToURL = gdi.isChecked("ToURL");
  sendToFile = gdi.isChecked("ToFile");

  cmpId = gdi.getTextNo("CmpID");
  passwd = gdi.getText("Password");
  prefix = gdi.getText("Prefix");
  exportScript = gdi.getText("ExportScript");
  zipFile = gdi.isChecked("Zip");

  ListBoxInfo lbi;
  gdi.getSelectedItem("Format", lbi);
  dataType = lbi.data;

  gdi.getSelection("Classes", classes);
  if (sendToFile) {
    if (folder.empty()) {
      throw meosException("Mappnamnet f�r inte vara tomt.");
    }

    if (*folder.rbegin() == '/' || *folder.rbegin() == '\\')
      folder = folder.substr(0, folder.size() - 1);

    file = folder;
    string exp = getExportFileName();
    if (fileExist(exp.c_str()))
      throw meosException(string("Filen finns redan: X#") + exp);
  }

  if (sendToURL) {
    if (xurl.empty()) {
      throw meosException("URL m�ste anges.");
    }
    url = xurl;
  }

  vector<pControl> ctrl;
  oe.getControls(ctrl, true);
  for (size_t k = 0; k< ctrl.size(); k++) {
    vector<int> ids;
    ctrl[k]->getCourseControls(ids);
    for (size_t i = 0; i < ids.size(); i++) {
      string id =  "C"+itos(ids[i]);
      if (gdi.hasField(id)) {
        bool st = gdi.isChecked(id);
        if (st != ctrl[k]->isValidRadio())
          ctrl[k]->setRadio(st);
      }
    }
  }

  process(gdi, &oe, SyncNone);

  interval = iv;
  synchronize = true;
  synchronizePunches = true;
}

void OnlineResults::status(gdioutput &gdi)
{
  gdi.addString("", 1, name);
  gdi.fillRight();
  gdi.pushX();
  if (sendToFile) {
    gdi.addString("", 0, "Mapp:");
    gdi.addStringUT(0, file);
    gdi.popX();
    gdi.dropLine(1);
  }
  if (sendToURL) {
    gdi.addString("", 0, "URL:");
    gdi.addStringUT(0, url);
    gdi.popX();
    gdi.dropLine(1);
  }

  if (sendToFile || sendToURL) {
    gdi.addString("", 0, "Exporterar om: ");
    gdi.addTimer(gdi.getCY(),  gdi.getCX(), timerIgnoreSign, (GetTickCount()-timeout)/1000);
    gdi.addString("", 0, "Antal skickade uppdateringar X (Y kb)#" +
                          itos(exportCounter-1) + "#" + itos(bytesExported/1024));
  }
  gdi.popX();

  gdi.dropLine(2);
  gdi.addButton("Stop", "Stoppa automaten", AutomaticCB).setExtra(getId());
  gdi.fillDown();
  gdi.addButton("OnlineResults", "Inst�llningar...", AutomaticCB).setExtra(getId());
  gdi.popX();
}

void OnlineResults::process(gdioutput &gdi, oEvent *oe, AutoSyncType ast) {
  errorLines.clear();
  DWORD tick = GetTickCount();
  if (lastSync + interval * 1000 > tick)
    return;

  if (!sendToFile && !sendToURL)
    return;
  ProgressWindow pwMain((sendToURL && ast == SyncNone) ? gdi.getHWND() : 0);
  pwMain.init();

  string t;
  int xmlSize = 0;
  InfoCompetition &ic = getInfoServer();
  xmlbuffer xmlbuff;
  if (dataType == 1) {
    if (ic.synchronize(*oe, classes)) {
      lastSync = tick; // If error, avoid to quick retry
      ic.getDiffXML(xmlbuff);
    }
  }
  else {
    t = getTempFile();
    if (dataType == 2)
      oe->exportIOFSplits(oEvent::IOF20, t.c_str(), false, false,
                          classes, -1, false, true, true, false);
    else if (dataType == 3)
      oe->exportIOFSplits(oEvent::IOF30, t.c_str(), false, false, 
                          classes, -1, false, true, true, false);
    else
      throw meosException("Internal error");
  }

  if (!t.empty() || xmlbuff.size() > 0) {
    if (sendToFile) {

      if (xmlbuff.size() > 0) {
        t = getTempFile();
        xmlparser xml(gdi.getEncoding() == ANSI ? 0 : &gdi);
        if (sendToURL) {
          xmlbuffer bcopy = xmlbuff;
          bcopy.startXML(xml, t);
          bcopy.commit(xml, xmlbuff.size());
        }
        else {
          xmlbuff.startXML(xml, t);
          xmlbuff.commit(xml, xmlbuff.size());
        }
        xml.endTag();
        xmlSize = xml.closeOut();

      }
      string fn = getExportFileName();

      if (!CopyFile(t.c_str(), fn.c_str(), false))
        gdi.addInfoBox("", "Kunde inte skriva resultat till X#" + fn);
      else if (!sendToURL) {
        ic.commitComplete();
        bytesExported +=xmlSize;
        removeTempFile(t);
      }

      if (!exportScript.empty()) {
        ShellExecute(NULL, NULL, exportScript.c_str(), fn.c_str(), NULL, SW_HIDE);
      }
    }

    try {
      if (sendToURL) {
        Download dwl;
        dwl.initInternet();
        ProgressWindow pw(0);
        vector<pair<string,string> > key;
		pair<string, string> mk1("competition", itos(cmpId));
        key.push_back(mk1);
		pair<string, string> mk2("pwd", passwd);
        key.push_back(mk2);

        bool moreToWrite = true;
        string tmp;
        const int total = max(xmlbuff.size(), 1u);

        while(moreToWrite) {

          t = getTempFile();
          xmlparser xmlOut(gdi.getEncoding() == ANSI ? 0 : &gdi);
          xmlbuff.startXML(xmlOut, t);
          moreToWrite = xmlbuff.commit(xmlOut, 250);
          xmlOut.endTag();
          xmlSize = xmlOut.closeOut();
          string result = getTempFile();

          if (zipFile && xmlSize > 1024) {
            string zipped = getTempFile();
            zip(zipped.c_str(), 0, vector<string>(1, t));
            removeTempFile(t);
            t = zipped;

            struct stat st;
            stat(t.c_str(), &st);
            bytesExported += st.st_size;
          }
          else
            bytesExported +=xmlSize;

          dwl.postFile(url, t, result, key, pw);
          removeTempFile(t);

          pwMain.setProgress(1000-(1000 * xmlbuff.size())/total);

          xmlparser xml(0);
          xmlobject res;
          try {
            xml.read(result);
            res = xml.getObject("MOPStatus");
          }
          catch(std::exception &) {
            ifstream is(result.c_str());
            is.seekg (0, is.end);
            int length = is.tellg();
            is.seekg (0, is.beg);
            char * buffer = new char [length+1];
            is.read (buffer,length);
            is.close();
            removeTempFile(result);
            buffer[length] = 0;
            OutputDebugString(buffer);
            split(buffer, "\n", errorLines);
            delete[] buffer;
            formatError(gdi);
            throw meosException("Onlineservern svarade felaktigt.");
          }
          removeTempFile(result);

          if (res)
            res.getObjectString("status", tmp);
          if (tmp == "BADCMP")
            throw meosException("Onlineservern svarade: Felaktigt t�vlings-id");
          if (tmp == "BADPWD")
            throw meosException("Onlineservern svarade: Felaktigt l�senord");
          if (tmp == "NOZIP")
            throw meosException("Onlineservern svarade: ZIP st�ds ej");
          if (tmp == "ERROR")
            throw meosException("Onlineservern svarade: Serverfel");

          if (tmp != "OK")
            break;
        }

        if (tmp == "OK")
          ic.commitComplete();
        else
          throw meosException("Misslyckades med att ladda upp onlineresultat");
      }
    }
    catch(std::exception &ex) {
      if (ast == SyncNone)
        throw;
      else
        gdi.addInfoBox("", string("Online Results Error X#")+ex.what(), 5000);
    }

    lastSync = GetTickCount();
    exportCounter++;
  }
}

void OnlineResults::formatError(gdioutput &gdi) {
  gdi.restore("ServerError", false);
  if (errorLines.empty()) {
    gdi.refresh();
    return;
  }
  gdi.setRestorePoint("ServerError");
  gdi.dropLine();
  gdi.fillDown();
  gdi.addString("", boldText, "Server response X:#" + getLocalTime()).setColor(colorRed);
  for (size_t k = 0; k < errorLines.size(); k++)
    gdi.addStringUT(0, errorLines[k]);
  gdi.scrollToBottom();
  gdi.refresh();
}

InfoCompetition &OnlineResults::getInfoServer() const {
  if (!infoServer)
    infoServer = new InfoCompetition(1);

  return *infoServer;
}

string OnlineResults::getExportFileName() const {
  char bf[260];
  sprintf_s(bf, "%s\\%s%04d.xml", file.c_str(), prefix.c_str(), exportCounter);
  return bf;
}