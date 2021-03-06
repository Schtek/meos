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

// oEvent.cpp: implementation of the oEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <vector>
#include <algorithm>

#include "oEvent.h"
#include "gdioutput.h"
#include "gdifonts.h"
#include "meosdb/sqltypes.h"
#include "meosexception.h"
#include "inthashmap.h"

#include "oDataContainer.h"
#include "csvparser.h"
#include "oFreeImport.h"

#include "random.h"
#include "SportIdent.h"
#include "RunnerDB.h"
#include "meos_util.h"
#include "meos.h"
#include "importformats.h"

#include <io.h>
#include <fcntl.h>
#include "localizer.h"
#include "iof30interface.h"

#include "meosdb/sqltypes.h"

string conv_is(int i)
{
  char bf[256];
//  if (i==0)
//    return "";
  //else
   if (_itoa_s(i, bf, 10)==0)
    return bf;
   return "";
}


int ConvertStatusToOE(int i)
{
  switch(i)
  {
      case StatusOK:
      return 0;
      case StatusDNS:  // Ej start
      return 1;
      case StatusDNF:  // Utg.
      return 2;
      case StatusMP:  // Felst.
      return 3;
      case StatusDQ: //Disk
      return 4;
      case StatusMAX: //Maxtid
      return 5;
  }
  return 1;//Ej start...?!
}

string &getFirst(string &inout, int maxNames) {
  int s = inout.size();
  for (int k = 0;k<s; k++) {
    if (inout[k] == ' ' && k>3 && maxNames<=1) {
      inout[k] = 0;
      maxNames--;
      return inout;
    }
  }
  return inout;
}

bool oEvent::exportOECSV(const char *file, int languageTypeIndex, bool includeSplits)
{
  enum {
    OEstno = 0, OEcard = 1, OEid = 2, OEsurname = 3, OEfirstname = 4,
    OEbirth = 5, OEsex = 6, OEnc = 8, OEstart = 9, OEfinish = 10, OEtime = 11, OEstatus = 12,
    OEclubno = 13, OEclub = 14, OEclubcity = 15, OEnat = 16, OEclassno = 17,
    OEclassshortname = 18, OEclassname = 19, OErent = 35, OEfee = 36, OEpaid = 37, OEcourseno = 38, OEcourse = 39,
    OElength = 40, OEclimb = 41, OEcoursecontrols = 42, OEpl = 43, OEstartpunch = 44, OEfinishpunch = 45
  };

  csvparser csv;

  oClass::initClassId(*this);

  if (!csv.openOutput(file))
    return false;

  calculateResults(RTClassResult);

  oRunnerList::iterator it;
  string maleString;
  string femaleString;

  switch (languageTypeIndex)
  {
  case 1: // English
    csv.OutputRow("Stno;Chip;Database Id;Surname;First name;YB;S;Block;nc;Start;Finish;Time;Classifier;Club no.;Cl.name;City;Nat;Cl. no.;Short;Long;Num1;Num2;Num3;Text1;Text2;Text3;Adr. name;Street;Line2;Zip;City;Phone;Fax;EMail;Id/Club;Rented;Start fee;Paid;Course no.;Course;km;m;Course controls;Pl;Start punch;Finish punch;Control1;Punch1;Control2;Punch2;Control3;Punch3;Control4;Punch4;Control5;Punch5;Control6;Punch6;Control7;Punch7;Control8;Punch8;Control9;Punch9;Control10;Punch10;(may be more) ...");
    maleString = "M";
    femaleString = "F";
    break;
  case 2: // Svenska
    csv.OutputRow("Startnr;Bricka;Databas nr.;Efternamn;F�rnamn;�r;K;Block;ut;Start;M�l;Tid;Status;Klubb nr.;Namn;Ort;Land;Klass nr.;Kort;L�ng;Num1;Num2;Num3;Text1;Text2;Text3;Adr. namn;Gata;Rad 2;Post nr.;Ort;Tel;Fax;E-post;Id/Club;Hyrd;Startavgift;Betalt;Bana nr.;Bana;km;Hm;Bana kontroller;Pl;Startst�mpling;M�lst�mpling;Kontroll1;St�mplar1;Kontroll2;St�mplar2;Kontroll3;St�mplar3;Kontroll4;St�mplar4;Kontroll5;St�mplar5;Kontroll6;St�mplar6;Kontroll7;St�mplar7;Kontroll8;St�mplar8;Kontroll9;St�mplar9;Kontroll10;St�mplar10;(kan forts�tta)..");
    maleString = "M"; 
    femaleString = "K"; 
    break;
  case 3: // Deutsch
    csv.OutputRow("Stnr;Chip;Datenbank Id;Nachname;Vorname;Jg;G;Block;AK;Start;Ziel;Zeit;Wertung;Club-Nr.;Abk;Ort;Nat;Katnr;Kurz;Lang;Num1;Num2;Num3;Text1;Text2;Text3;Adr. Name;Stra�e;Zeile2;PLZ;Ort;Tel;Fax;EMail;Id/Verein;Gemietet;Startgeld;Bezahlt;Bahnnummer;Bahn;km;Hm;Bahn Posten;Pl;Startstempel;Zielstempel;Posten1;Stempel1;Posten2;Stempel2;Posten3;Stempel3;Posten4;Stempel4;Posten5;Stempel5;Posten6;Stempel6;Posten7;Stempel7;Posten8;Stempel8;Posten9;Stempel9;Posten10;Stempel10;(und weitere)...");
    maleString = "M";
    femaleString = "W";
    break;
  case 4: // Dansk
    csv.OutputRow("Stnr;Brik;Database ID;Efternavn;Fornavn;�r;K;Blok;UFK;Start;M�l;Tid;Status;Klub nr.;Navn;Klub;Land;Klasse nr.;kort;Lang;Num1;Num2;Num3;Text1;Text2;Text3;Adr. navn;Gade;Linie2;Post nr.;Klub;Tlf.;Fax.;Email;Id/klub;Lejet;Startafgift;Betalt;Bane nr.;Bane;km;Hm;Poster p� bane;Pl;Start-stempling;M�l-stempling;Post1;Klip1;Post2;Klip2;Post3;Klip3;Post4;Klip4;Post5;Klip5;Post6;Klip6;Post7;Klip7;Post8;Klip8;Post9;Klip9;Post10;Klip10;(m�ske mere)...");
    maleString = "M";
    femaleString = "K";
    break;
  case 5: // Fran�ais
    csv.OutputRow("N� d�p.;Puce;Ident. base de donn�es;Nom;Pr�nom;N�;S;Plage;nc;D�part;Arriv�e;Temps;Evaluation;N� club;Nom;Ville;Nat;N� cat.;Court;Long;Num1;Num2;Num3;Text1;Text2;Text3;Adr. nom;Rue;Ligne2;Code Post.;Ville;T�l.;Fax;E-mail;Id/Club;Lou�e;Engagement;Pay�;Circuit N�;Circuit;km;m;Postes du circuit;Pl;Poin�on de d�part;Arriv�e (P);Poste1;Poin�on1;Poste2;Poin�on2;Poste3;Poin�on3;Poste4;Poin�on4;Poste5;Poin�on5;Poste6;Poin�on6;Poste7;Poin�on7;Poste8;Poin�on8;Poste9;Poin�on9;Poste10;Poin�on10;(peut �tre plus) ...");
    maleString = "H";
    femaleString = "F";
    break;
  case 6: // Russian
    csv.OutputRow("Stnr;Chip;Datenbank Id;Nachname;Vorname;Jg;G_Sex;Block;AK_notclass;Start;Ziel;Zeit;Wertung;Club-Nr.;Abk;Ort;Nat;Katnr;Kurz;Lang;Num1;Num2;Num3;Text1;Text2;Text3;Adr. Name;Strasse;Zeile2;PLZ;Ort;Tel;Fax;EMail;Club_TIdNr;Gemietet;Startgeld;Bezahlt;Bahnnummer;Bahn;km_Kilometer;Hm_Climbmeter;Bahn Posten;Pl_Place;Startstempel;Zielstempel;Posten1;Stempel1;Posten2;Stempel2;Posten3;Stempel3;Posten4;Stempel4;Posten5;Stempel5;Posten6;Stempel6;Posten7;Stempel7;Posten8;Stempel8;Posten9;Stempel9;Posten10;Stempel10;(und weitere)...");
    maleString = "M";
    femaleString = "W";
    break;
  default:
    csv.OutputRow("Stno;Chip;Database Id;Surname;First name;YB;S;Block;nc;Start;Finish;Time;Classifier;Club no.;Cl.name;City;Nat;Cl. no.;Short;Long;Num1;Num2;Num3;Text1;Text2;Text3;Adr. name;Street;Line2;Zip;City;Phone;Fax;EMail;Id/Club;Rented;Start fee;Paid;Course no.;Course;km;m;Course controls;Pl;Start punch;Finish punch;Control1;Punch1;Control2;Punch2;Control3;Punch3;Control4;Punch4;Control5;Punch5;Control6;Punch6;Control7;Punch7;Control8;Punch8;Control9;Punch9;Control10;Punch10;(may be more) ...");
    maleString = "M";
    femaleString = "F";
  }

  char bf[256];
  for (it = Runners.begin(); it != Runners.end(); ++it) {
    vector<string> row;
    row.resize(46);
    oDataInterface di = it->getDI();

    row[OEstno] = conv_is(it->getId());
    row[OEcard] = conv_is(it->getCardNo());
    if (it->getExtIdentifier() != 0)
      row[OEid] = it->getExtIdentifierString();
    row[OEsurname] = it->getFamilyName();
    row[OEfirstname] = it->getGivenName();
    row[OEbirth] = conv_is(di.getInt("BirthYear") % 100);

    // Specialized per language
    PersonSex s = it->getSex();
    switch (s) {
    case sFemale:
      row[OEsex] = femaleString;
      break;
    case sMale:
      row[OEsex] = maleString;
      break;
    case sBoth:
    case sUnknown:
    default:
      row[OEsex] = di.getString("Sex");
      break;
    }

    // nc / Runner shall not / doesn't want to be ranked
    if (it->getStatus() == StatusNotCompetiting)
      row[OEnc] = "X";
    else
      row[OEnc] = "0";

    // Excel format HH:MM:SS
    string dash = MakeDash("-");
    row[OEstart] = it->getStartTimeS();
    if (row[OEstart] == dash)
      row[OEstart] = "";

    // Excel format HH:MM:SS
    row[OEfinish] = it->getFinishTimeS();
    if (row[OEfinish] == dash)
      row[OEfinish] = "";

    // Excel format HH:MM:SS
    row[OEtime] = formatTimeHMS(it->getRunningTime());
    if (row[OEtime] == dash)
      row[OEtime] = "";

    row[OEstatus] = conv_is(ConvertStatusToOE(it->getStatus()));
    row[OEclubno] = conv_is(it->getClubId());

    if (it->getClubRef()) {
      row[OEclub] = it->getClubRef()->getDI().getString("ShortName");
      row[OEclubcity] = it->getClub();
    }
    row[OEnat] = di.getString("Nationality");
    row[OEclassno] = conv_is(it->getClassId());
    row[OEclassshortname] = it->getClass();
    row[OEclassname] = it->getClass();

    row[OErent] = conv_is(di.getInt("CardFee"));
    row[OEfee] = conv_is(di.getInt("Fee"));
    row[OEpaid] = conv_is(di.getInt("Paid"));

    pCourse pc = it->getCourse(true);
    if (pc) {
      row[OEcourseno] = conv_is(pc->getId());
      row[OEcourse] = pc->getName();
      if (pc->getLength()>0) {
        sprintf_s(bf, "%d.%d", pc->getLength() / 1000, pc->getLength() % 1000);
        row[OElength] = bf;
      }
      row[OEclimb] = conv_is(pc->getDI().getInt("Climb"));

      row[OEcoursecontrols] = conv_is(pc->nControls);
    }
    row[OEpl] = it->getPlaceS();

    if (includeSplits && pc != NULL)
    {
      // Add here split times

      // row[45]: finish time
      row[OEfinishpunch] = row[OEfinish];

      // row[46; 48; 50; ..]: control id
      // row[47; 49; 51; ..]: punch time of control id row[i-1]

      const vector<SplitData> &sp = it->getSplitTimes(true);

      bool hasRogaining = pc->hasRogaining();
      int startIx = pc->useFirstAsStart() ? 1 : 0;
      int endIx = pc->useLastAsFinish() ? pc->nControls - 1 : pc->nControls;

      for (int k = startIx, m = 0; k < endIx; k++, m += 2) {
        if (pc->getControl(k)->isRogaining(hasRogaining))
          continue;
        row.push_back(pc->getControl(k)->getIdS());
        if (unsigned(k) < sp.size() && sp[k].time > 0)
          row.push_back(formatTimeHMS(sp[k].time - it->tStartTime));
        else
          row.push_back("-----");
      }

      // Extra punches
      vector<pFreePunch> punches;

      oe->getPunchesForRunner(it->getId(), punches);
      for (vector<pFreePunch>::iterator punchIt = punches.begin(); punchIt != punches.end(); ++punchIt) {
        pPunch punch = *punchIt;
        if (!punch->isUsed && !(punch->isFinish() && !pc->useLastAsFinish()) && !(punch->isStart() && !pc->useFirstAsStart()) && !punch->isCheck())
        {
          row.push_back(punch->getType());

          int t = punch->getAdjustedTime();
          if (it->tStartTime > 0 && t > 0 && t > it->tStartTime)
            row.push_back(formatTimeHMS(t - it->tStartTime));
          else
            return "-----";
        }
      }

    }

    csv.OutputRow(row);
  }

  csv.closeOutput();

  return true;
}

void oEvent::importXML_EntryData(gdioutput &gdi, const char *file, bool updateClass, bool removeNonexisting)
{
  vector< pair<int, int> > runnersInTeam;
  for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
    if (!it->isRemoved() && it->tInTeam) {
      runnersInTeam.push_back(make_pair(it->getId(), it->getClassId()) );
    }
  }

  xmlparser xml(0);
  xml.read(file);

  xmlobject xo = xml.getObject("EntryList");

  if (xo) {

    gdi.addString("", 0, "Importerar anm�lningar (IOF, xml)");
    gdi.refreshFast();
    int ent = 0, fail = 0, removed = 0;

    if (xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readEntryList(gdi, xo, removeNonexisting, ent, fail, removed);
    }
    else {
      xmlList xl;
      xo.getObjects(xl);
      xmlList::const_iterator it;

      for(it=xl.begin(); it != xl.end(); ++it){
        if (it->is("ClubEntry")){
          xmlList entries;
          //xmlobject xentry=it->getObject("Entry");
          int ClubId = 0;

          xmlobject club = it->getObject("Club");

          if (club) {
            addXMLClub(club, false);
            ClubId = club.getObjectInt("ClubId");
          }
          else
            ClubId = it->getObjectInt("ClubId");

          it->getObjects("Entry", entries);
          for (size_t k = 0; k<entries.size(); k++) {
            bool team = entries[k] && entries[k].getObject("TeamName");
            if (team) {
              if (addXMLTeamEntry(entries[k], ClubId))
                ent++;
              else
                fail++;
            }
            else {
              if (addXMLEntry(entries[k], ClubId, true))
                ent++;
              else
                fail++;
            }
          }
        }
      }
    }
    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(ent));
    if (fail>0)
      gdi.addString("", 0, "Antal misslyckade: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
  }


  xo = xml.getObject("StartList");

  if (xo) {

    gdi.addString("", 0, "Importerar anm�lningar (IOF, xml)");
    gdi.refreshFast();

    int ent = 0, fail = 0;

    if (xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readStartList(gdi, xo, ent, fail);
    }
    else {
      xmlList xl;
      xo.getObjects(xl);

      xmlList::const_iterator it;
      for(it=xl.begin(); it != xl.end(); ++it){
        if (it->is("ClassStart")){
          xmlList entries;
          int clsId = it->getObjectInt("ClassId");

          pClass cls = 0;
          if (clsId == 0) {
            string clsName;
            it->getObjectString("ClassShortName", clsName);
            if (!clsName.empty())
              cls = getClassCreate(0, clsName);
          }
          else
            cls = getClassCreate(clsId, lang.tl("Klass ") + itos(clsId));

          it->getObjects("PersonStart", entries);
          for (size_t k = 0; k<entries.size(); k++) {
            {
              if (addXMLStart(entries[k], cls))
                ent++;
              else
                fail++;
            }
          }
        }
      }
    }
    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(ent));
    if (fail>0)
      gdi.addString("", 0, "Antal misslyckade: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
  }

  xo = xml.getObject("ClassData");

  if (!xo)
    xo = xml.getObject("ClassList");

  if (xo) {
    gdi.addString("", 0, "Importerar klasser (IOF, xml)");
    gdi.refreshFast();
    int imp = 0, fail = 0;

    if (xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readClassList(gdi, xo, imp, fail);
    }
    else {
      xmlList xl;
      xo.getObjects(xl);

      xmlList::const_iterator it;

      for (it=xl.begin(); it != xl.end(); ++it) {
        if (it->is("Class")) {
          if (addXMLClass(*it))
            imp++;
          else fail++;
        }
      }
    }
    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(imp));
    if (fail>0)
      gdi.addString("", 0, "Antal misslyckade: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
  }

  xo=xml.getObject("ClubList");

  if (xo) {
    gdi.addString("", 0, "Importerar klubbar (IOF, xml)");
    gdi.refreshFast();
    int imp = 0, fail = 0;

    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;

    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Club")){
        if (addXMLClub(*it, false))
          imp++;
        else
          fail++;
      }
    }
    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(imp));
    if (fail>0)
      gdi.addString("", 0, "Antal misslyckade: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
  }

  xo=xml.getObject("RankList");

  if (xo) {
    gdi.addString("", 0, "Importerar ranking (IOF, xml)");
    gdi.refreshFast();
    int imp = 0, fail = 0;

    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;

    map<__int64, int> ext2Id;
    for (oRunnerList::iterator it = Runners.begin(); it != Runners.end(); ++it) {
      if (it->skip())
        continue;
      __int64 ext = it->getExtIdentifier();
      if (ext != 0)
        ext2Id[ext] = it->getId();
    }

    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Competitor")){
        if (addXMLRank(*it, ext2Id))
          imp++;
        else
          fail++;
      }
    }

    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(imp));
    if (fail>0)
      gdi.addString("", 0, "Antal ignorerade: X#" + itos(fail));
    gdi.dropLine();
    gdi.refreshFast();
  }

  xo=xml.getObject("CourseData");

  if (xo) {
    gdi.addString("", 0, "Importerar banor (IOF, xml)");
    gdi.refreshFast();
    int imp = 0, fail = 0;

    if (xo && xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readCourseData(gdi, xo, updateClass, imp, fail);
    }
    else {
      xmlList xl;
      xo.getObjects(xl);

      xmlList::const_iterator it;

      for(it=xl.begin(); it != xl.end(); ++it){
        if (it->is("Course")){
          if (addXMLCourse(*it, updateClass))
            imp++;
          else
            fail++;
        }
        else if (it->is("Control")){
          addXMLControl(*it, 0);
        }
        else if (it->is("StartPoint")){
          addXMLControl(*it, 1);
        }
        else if (it->is("FinishPoint")){
          addXMLControl(*it, 2);
        }
      }
    }

    gdi.addString("", 0, "Klart. Antal importerade: X#" + itos(imp));
    if (fail>0)
      gdi.addString("", 0, "Antal misslyckade: X#" + itos(fail)).setColor(colorRed);
    gdi.dropLine();
    gdi.refreshFast();
  }


  xo=xml.getObject("EventList");

  if (xo) {
    gdi.addString("", 0, "Importerar t�vlingsdata (IOF, xml)");
    gdi.refreshFast();

    if (xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readEventList(gdi, xo);
      gdi.addString("", 0, "T�vlingens namn: X#" + getName());
      gdi.dropLine();
      gdi.refreshFast();
    }
    else {
      xmlList xl;
      xo.getObjects(xl);

      xmlList::const_iterator it;

      for(it=xl.begin(); it != xl.end(); ++it){
        if (it->is("Event")){
          addXMLEvent(*it);
          gdi.addString("", 0, "T�vlingens namn: X#" + getName());
          gdi.dropLine();
          gdi.refreshFast();
          break;
        }
      }
    }
  }

  vector<int> toRemove;
  for (size_t k = 0; k < runnersInTeam.size(); k++) {
    int id = runnersInTeam[k].first;
    int classId = runnersInTeam[k].second;
    pRunner r = getRunner(id, 0);
    if (r && !r->tInTeam && r->getClassId() == classId) {
      toRemove.push_back(r->getId());
    }
  }
  removeRunner(toRemove);
}

bool oEvent::addXMLCompetitorDB(const xmlobject &xentry, int clubId)
{
  if (!xentry) return false;

  xmlobject person = xentry.getObject("Person");
  if (!person) return false;

  string pids;
  person.getObjectString("PersonId", pids);
  __int64 extId = oBase::converExtIdentifierString(pids);
  int pid = oBase::idFromExtId(extId);

  xmlobject pname = person.getObject("PersonName");
  if (!pname) return false;

  int cardno = 0;
  string tmp;

  xmlList cards;
  xentry.getObjects("CCard", cards);

  for (size_t k = 0; k<cards.size(); k++) {
    xmlobject &card = cards[k];
    if (card) {
      xmlobject psys = card.getObject("PunchingUnitType");
      if (!psys || psys.getObjectString("value", tmp) == "SI") {
        cardno = card.getObjectInt("CCardId");
        break;
      }
    }
  }

  //if (!cardno)
  //  return false;

  string given;
  pname.getObjectString("Given", given);
  getFirst(given, 2);
  string family;
  pname.getObjectString("Family", family);

  if (given.empty() || family.empty())
    return false;

  string name(family + ", " + given);

  char sex[2];
  person.getObjectString("sex", sex, 2);

  xmlobject bd = person.getObject("BirthDate");
  int birth = bd ? bd.getObjectInt("Date") : 0;

  xmlobject nat = person.getObject("Nationality");

  char national[4] = { 0,0,0,0 };
  if (nat) {
    xmlobject natId = nat.getObject("CountryId");
    if (natId)
      natId.getObjectString("value", national, 4);
  }

  RunnerDBEntry *rde = runnerDB->getRunnerById(extId);

  if (!rde) {
    rde = runnerDB->getRunnerByCard(cardno);

    if (rde && rde->getExtId() != 0)
      rde = 0; //Other runner, same card

    if (!rde)
      rde = runnerDB->addRunner(name.c_str(), pid, clubId, cardno);
  }

  if (rde) {
    rde->setExtId(extId);
    rde->setName(name.c_str());
    rde->clubNo = clubId;
    rde->birthYear = extendYear(birth);
    rde->sex = sex[0];
    memcpy(rde->national, national, 3);
  }
  return true;
}

bool oEvent::addOECSVCompetitorDB(const vector<string> &row)
{
  // Ident. base de donn�es;Puce;Nom;Pr�nom;N�;S;N� club;Nom;Ville;Nat;N� cat.;Court;Long;Num1;Num2;Num3;E_Mail;Texte1;Texte2;Texte3;Adr. nom;Rue;Ligne2;Code Post.;Ville;T�l.;Fax;E-mail;Id/Club;Lou�e
  enum { OEid = 0, OEcard = 1, OEsurname = 2, OEfirstname = 3, OEbirth = 4, OEsex = 5,
    OEclubno = 6, OEclub = 7, OEclubcity = 8, OEnat = 9, OEclassno = 10, OEclassshort = 11, OEclasslong = 12
  };

  int pid = atoi(row[OEid].c_str());

  string given = row[OEfirstname];
  string family = row[OEsurname];

  if (given.empty() && family.empty())
    return false;

  string name = family + ", " + given;
  
  // Depending on the OE language, man = "H" (French) or "M" (English, Svenska, Dansk, Russian, Deutsch)
  // woman = "F" (English, French) or "W" (Deutsch, Russian) or "K" (Svenska, Dansk)
  char sex[2];
  if (row[OEsex] == "H" || row[OEsex] == "M")
    strcpy(sex, "M");
  else if (row[OEsex] == "F" || row[OEsex] == "K" || row[OEsex] == "W")
    strcpy(sex, "W");
  else
    strcpy(sex, "");

  int birth = atoi(row[OEbirth].c_str());

  // Hack to take care of inconsistency between FFCO licensees archive (France) and event registrations from FFCO (FR)
  char national[4] = { 0,0,0,0 };
  if (row[OEnat] == "France") {
    strcpy(national, "FRA");
  }

  // Extract club data

  int clubId = atoi(row[OEclubno].c_str());
  string clubName;
  string shortClubName;

  clubName = row[OEclubcity];
  shortClubName = row[OEclub];

  if (clubName.length() > 0 && IsCharAlphaNumeric(clubName[0])) {

    pClub pc = new oClub(this);
    pc->Id = clubId;

    pc->setName(clubName);
    pc->setExtIdentifier(clubId);

    oDataInterface DI = pc->getDI();
    DI.setString("ShortName", shortClubName.substr(0, 8));
    // Nationality?

    runnerDB->importClub(*pc, false);
    delete pc;
  }

  RunnerDBEntry *rde = runnerDB->getRunnerById(pid);

  int cardno = atoi(row[OEcard].c_str());
  if (!rde) {
    rde = runnerDB->getRunnerByCard(cardno);

    if (rde && rde->getExtId() != 0)
      rde = NULL; //Other runner, same card

    if (!rde)
      rde = runnerDB->addRunner(name.c_str(), pid, clubId, cardno);
  }

  if (rde) {
    rde->setExtId(pid);
    rde->setName(name.c_str());
    rde->clubNo = clubId;
    rde->birthYear = extendYear(birth);
    rde->sex = sex[0];
    memcpy(rde->national, national, 3);
  }
  return true;
}

bool oEvent::addXMLTeamEntry(const xmlobject &xentry, int clubId)
{
  if (!xentry) return false;

  string name;
  xentry.getObjectString("TeamName", name);
  int id = xentry.getObjectInt("EntryId");

  if (name.empty())
    name = lang.tl("Lag X#" + itos(id));

  xmlList teamCmp;
  xentry.getObjects("TeamCompetitor", teamCmp);

  xmlobject cls = xentry.getObject("EntryClass");
  xmlobject edate = xentry.getObject("EntryDate");

  pClass pc = getXMLClass(xentry);

  if (!pc)
    return false;

  pClub club = getClubCreate(clubId);

  pTeam t = getTeam(id);

  if (t == 0) {
    if ( id > 0) {
      oTeam or(this, id);
      t = addTeam(or, true);
    }
    else {
      oTeam or(this);
      t = addTeam(or, true);
    }
    t->setStartNo(Teams.size(), false);
  }

  if (!t->hasFlag(oAbstractRunner::FlagUpdateName))
    t->setName(name, false);
  if (!t->Class || !t->hasFlag(oAbstractRunner::FlagUpdateClass))
    t->setClassId(pc->getId(), false);

  t->Club = club;
  oDataInterface DI = t->getDI();

  string date;
  if (edate) DI.setDate("EntryDate", edate.getObjectString("Date", date));

  int maxleg = teamCmp.size();
  for (size_t k = 0; k < teamCmp.size(); k++) {
    maxleg = max(maxleg, teamCmp[k].getObjectInt("TeamSequence"));
  }

  if (pc->getNumStages() < unsigned(maxleg)) {
    setupRelay(*pc, PRelay, maxleg, getAbsTime(3600));
  }

  for (size_t k = 0; k < teamCmp.size(); k++) {
    int leg = teamCmp[k].getObjectInt("TeamSequence") - 1;
    if (leg>=0) {
      pRunner r = addXMLEntry(teamCmp[k], clubId, false);
      t->setRunner(leg, r, true);
    }
  }

  return true;
}

pClass oEvent::getXMLClass(const xmlobject &xentry) {

  xmlobject eclass = xentry.getObject("EntryClass");
  if (eclass) {
    int cid = eclass.getObjectInt("ClassId");
    pClass pc = getClass(cid);
    if ( pc == 0 && cid>0) {
      oClass cls(this, cid);
      cls.Name = lang.tl("Klass X#" + itos(cid));
      pc = addClass(cls);//Create class if not found
    }
    return pc;
  }
  return 0;
}

pClub oEvent::getClubCreate(int clubId) {
  if (clubId) {
    pClub club = getClub(clubId);
    if (!club) {
      pClub dbClub = runnerDB->getClub(clubId);
      if (dbClub) {
        club = addClub(*dbClub);
      }
      if (!club) {
        club = addClub(lang.tl("Klubb X#" + itos(clubId)));
      }
    }
    return club;
  }
  return 0;
}

pRunner oEvent::addXMLPerson(const xmlobject &person) {
  xmlobject pname = person.getObject("PersonName");
  if (!pname) return 0;

  string pids;
  person.getObjectString("PersonId", pids);
  __int64 extId = oBase::converExtIdentifierString(pids);
  int pid = oBase::idFromExtId(extId);
  pRunner r = 0;

  if (pid)
    r = getRunner(pid, 0);

  if (!r) {
    if ( pid > 0) {
      oRunner or(this, pid);
      r = addRunner(or, true);
    }
    else {
      oRunner or(this);
      r = addRunner(or, true);
    }
  }

  string given, family;
  pname.getObjectString("Given", given);
  pname.getObjectString("Family", family);

  r->setName(family + ", " + getFirst(given, 2), false);
  r->setExtIdentifier(extId);

  oDataInterface DI=r->getDI();
  string tmp;

  r->setSex(interpretSex(person.getObjectString("sex", tmp)));
  xmlobject bd=person.getObject("BirthDate");

  if (bd) r->setBirthYear(extendYear(bd.getObjectInt("Date")));

  xmlobject nat=person.getObject("Nationality");

  if (nat) {
    char national[4];
    xmlobject natId = nat.getObject("CountryId");
    if (natId)
      r->setNationality(natId.getObjectString("value", national, 4));
  }

  return r;
}

pRunner oEvent::addXMLEntry(const xmlobject &xentry, int clubId, bool setClass) {
  if (!xentry) return 0;

  xmlobject person = xentry.getObject("Person");
  if (!person) return 0;

  pRunner r = addXMLPerson(person);

  int cmpClubId = xentry.getObjectInt("ClubId");
  if (cmpClubId != 0)
    clubId = cmpClubId;

  oDataInterface DI=r->getDI();
  string given = r->getGivenName();
  xmlList cards;
  xentry.getObjects("CCard", cards);
  string tmp;
  for (size_t k= 0; k<cards.size(); k++) {
    xmlobject &card = cards[k];
    if (card) {
      xmlobject psys = card.getObject("PunchingUnitType");
      if (!psys || psys.getObjectString("value", tmp) == "SI") {
        int cardno = card.getObjectInt("CCardId");
        r->setCardNo(cardno, false);
        break;
      }
    }
  }

  pClass oldClass = r->Class;
  pClub oldClub = r->Club;

  if (setClass && !r->hasFlag(oAbstractRunner::FlagUpdateClass) )
    r->Class = getXMLClass(xentry);

  r->Club = getClubCreate(clubId);

  if (oldClass != r->Class || oldClub != r->Club)
    r->updateChanged();

  xmlobject edate=xentry.getObject("EntryDate");
  if (edate) DI.setDate("EntryDate", edate.getObjectString("Date", tmp));

  r->addClassDefaultFee(false);
  r->synchronize();

  xmlobject adjRunner = xentry.getObject("AllocationControl");

  if (adjRunner) {
    xmlobject person2 = adjRunner.getObject("Person");
    if (person2) {
      xmlobject pname2 = person2.getObject("PersonName");

      string pids2;
      person.getObjectString("PersonId", pids2);
      const __int64 extId2 = oBase::converExtIdentifierString(pids2);
      int pid2 = oBase::idFromExtId(extId2);
      pRunner r2 = getRunner(pid2, 0);

      if (!r2) {
        if ( pid2 > 0) {
          oRunner or(this, pid2);
          r2 = addRunner(or, true);
        }
        else {
          oRunner or(this);
          r2 = addRunner(or, true);
        }
      }

      string given2, family2;
      if (pname2) {
        pname2.getObjectString("Given", given2);
        pname2.getObjectString("Family", family2);
        r2->setName(family2 + ", " + getFirst(given2, 2), false);
      }

      r2->setExtIdentifier(pid2);
      // Create patrol

      if (r->Class) {

        bool createTeam = false;
        pClass pc = r->Class;
        if (pc->getNumStages() <= 1) {
          setupRelay(*pc, PPatrolOptional, 2, getAbsTime(3600));
          createTeam = true;
        }

        pTeam t = r->tInTeam;
        if (t == 0)
          t = r2->tInTeam;

        if (t == 0) {
          autoAddTeam(r);
          t = r2->tInTeam;
          createTeam = true;
        }

        if (t != 0) {
          if (t->getRunner(0) == r2) {
            t->setRunner(0, r2, true);
            t->setRunner(1, r, true);
          }
          else {
            t->setRunner(0, r, true);
            t->setRunner(1, r2, true);
          }
        }

        if (createTeam && t && !t->hasFlag(oAbstractRunner::FlagUpdateName)) {
          t->setName(given + " / " + given2, false);
        }
      }
    }
  }
  else {
    if (r->Class && r->Class->getNumStages()>=2) {
      autoAddTeam(r);
    }
  }
  return r;
}


pRunner oEvent::addXMLStart(const xmlobject &xstart, pClass cls) {

  if (!xstart) return 0;

  xmlobject person_ = xstart.getObject("Person");
  if (!person_) return 0;

  pRunner r = addXMLPerson(person_);
  pClass oldClass = r->Class;
  pClub oldClub = r->Club;
  r->Class = cls;

  xmlobject xclub = xstart.getObject("Club");
  int clubId = xstart.getObjectInt("ClubId");
  string cname;
  if (xclub) {
    clubId = xclub.getObjectInt("ClubId");
    xclub.getObjectString("ShortName", cname);
  }

  if (clubId > 0) {
    r->Club = getClubCreate(clubId, cname);
  }

  xmlobject xstrt = xstart.getObject("Start");

  if (!xstrt)
    return r;

  oDataInterface DI=r->getDI();

  int cardno = xstrt.getObjectInt("CCardId");

  if (cardno == 0) {
    xmlList cards;
    xstrt.getObjects("CCard", cards);
    string tmp;
    for (size_t k= 0; k<cards.size(); k++) {
      xmlobject &card = cards[k];
      if (card) {
        xmlobject psys = card.getObject("PunchingUnitType");
        if (!psys || psys.getObjectString("value", tmp) == "SI") {
          int cardno = card.getObjectInt("CCardId");
          r->setCardNo(cardno, false);
          break;
        }
      }
    }
  }
  else
    r->setCardNo(cardno, false);

  string tmp;
  xmlobject xstarttime = xstrt.getObject("StartTime");
  if (xstarttime)
    r->setStartTimeS(xstarttime.getObjectString("Clock", tmp));

  if (oldClass != r->Class || oldClub != r->Club)
    r->updateChanged();

  r->addClassDefaultFee(false);
  r->synchronize();

  if (r->Class && r->Class->getNumStages()>=2) {
    autoAddTeam(r);
  }
  return r;
}

bool addXMLPerson(const xmlobject &xentry, oWordList &givenNames,
                                           oWordList &familyNames) {
  if (!xentry) return false;

  xmlobject person=xentry.getObject("Person");
  if (!person) return false;

  xmlobject pname=person.getObject("PersonName");
  if (!pname) return false;

  char bf[256];
  givenNames.insert(pname.getObjectString("Given", bf, sizeof(bf)));
  familyNames.insert(pname.getObjectString("Family",  bf, sizeof(bf)));

  return true;
}

void importXMLNames(const xmlobject &xml, oWordList &givenNames,
                                  oWordList &familyNames)
{
  if (!xml)
    return;

  xmlobject xo = xml.getObject("CompetitorList");

  if (xo){
    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;

    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Competitor")){
        addXMLPerson(*it, givenNames, familyNames);
      }
    }
  }
}

void addXMLClub(const xmlobject *xclub, oWordList &clubs)
{
  if (!xclub)
    return;


  string Name;
  xclub->getObjectString("Name", Name);

  if (Name.empty() || !IsCharAlphaNumeric(Name[0]))
    return;

  clubs.insert(Name.c_str());
  return;
}

void importXMLAllClubs(const xmlobject &xml, oWordList &clubs)
{
  if (!xml)
    return;
  xmlobject xo=xml.getObject("ClubList");

  if (xo){
    xmlList xl;
    xo.getObjects(xl);

    xmlList::const_iterator it;

    for(it=xl.begin(); it != xl.end(); ++it){
      if (it->is("Club")){
        addXMLClub(&*it, clubs);
      }
    }
  }
}

bool oEvent::importXMLNames(const char *file,
                            oFreeImport &fi, string &info) const
{
  xmlparser xml(0);
  info.clear();
  DWORD tc=GetTickCount(), t;

  xml.read(file);
  char bf[128];
  t=GetTickCount()-tc;
  sprintf_s(bf, "XML read: %d.%ds ", t/1000, (t/100)%10);
  info+=bf;
  tc=GetTickCount();
  if (xml.getObject("meosdata")) {
    oEvent oNew(gdibase);
    oNew.open(xml);

    fi.init(oNew.Runners, oNew.Clubs, oNew.Classes);
  }
  else {
    ::importXMLNames(xml.getObject("CompetitorList"), fi.givenDB, fi.familyDB);
    ::importXMLAllClubs(xml.getObject("ClubList"), fi.clubDB);
  }

  t=GetTickCount()-tc;
  sprintf_s(bf, "Import: %d.%ds ", t/1000, (t/100)%10);
  info+=bf;

  return true;
}

void oEvent::importOECSV_Data(const char *oecsvfile, bool clear) {
  // Clear DB if needed
  if (clear) {
    runnerDB->clearClubs();
    runnerDB->clearRunners();
  }

  csvparser cp;
  list< vector<string> > data;

  gdibase.addString("",0,"L�ser l�pare...");
  gdibase.refresh();

  cp.parse(oecsvfile, data);

  gdibase.addString("", 0, "Behandlar l�pardatabasen").setColor(colorGreen);
  
  gdibase.refresh();

  list<vector<string>>::iterator it;

  for (it = ++(data.begin()); it != data.end(); ++it) {
    addOECSVCompetitorDB(*it);
  }
    
  gdibase.addString("", 0, "Klart. Antal importerade: X#" + itos(data.size()));
  gdibase.refresh();

  setProperty("DatabaseUpdate", getRelativeDay());

  // Save DB
  saveRunnerDatabase("database", true);

  if (HasDBConnection) {
    gdibase.addString("", 0, "Uppdaterar serverns databas...");
    gdibase.refresh();

    OpFailStatus stat = (OpFailStatus)msUploadRunnerDB(this);

    if (stat == opStatusFail) {
      char bf[256];
      msGetErrorState(bf);
      string error = string("Kunde inte ladda upp l�pardatabasen (X).#") + bf;
      throw meosException(error);
    }
    else if (stat == opStatusWarning) {
      char bf[256];
      msGetErrorState(bf);
      gdibase.addInfoBox("", string("Kunde inte ladda upp l�pardatabasen (X).#") + bf, 5000);
    }

    gdibase.addString("", 0, "Klart");
    gdibase.refresh();
  }
}

void oEvent::importXML_IOF_Data(const char *clubfile,
                              const char *competitorfile, bool clear)
{
  if (clubfile && clubfile[0]) {
    xmlparser xml_club(0);
    xml_club.setProgress(gdibase.getHWND());

    if (clear)
      runnerDB->clearClubs();

    gdibase.addString("",0,"L�ser klubbar...");
    gdibase.refresh();

    xml_club.read(clubfile);

    gdibase.addString("",0,"L�gger till klubbar...");
    gdibase.refresh();

    xmlobject xo = xml_club.getObject("ClubList");
    int clubCount = 0;

    if (!xo) {
      xo = xml_club.getObject("OrganisationList");
      if (xo) {
        IOF30Interface reader(this, false);
        reader.readClubList(gdibase, xo, clubCount);
      }
    }
    else {
      xmlList xl;
      if (xo)
        xo.getObjects(xl);

      xmlList::const_iterator it;
      for (it=xl.begin(); it != xl.end(); ++it)
        if (it->is("Club")) {
          if (addXMLClub(*it, true))
            clubCount++;
        }
    }
    gdibase.addStringUT(0, lang.tl("Antal importerade: ") + itos(clubCount));
  }

  if (competitorfile && competitorfile[0]) {
    xmlparser xml_cmp(0);
    xml_cmp.setProgress(gdibase.getHWND());
    gdibase.dropLine();
    gdibase.addString("",0,"L�ser l�pare...");
    gdibase.refresh();

    xml_cmp.read(competitorfile);

    if (clear) {
      runnerDB->clearRunners();
    }

    gdibase.addString("",0,"L�gger till l�pare...");
    gdibase.refresh();

    int personCount = 0;
    xmlobject xo=xml_cmp.getObject("CompetitorList");

    if (xo && xo.getAttrib("iofVersion")) {
      IOF30Interface reader(this, false);
      reader.readCompetitorList(gdibase, xo, personCount);
    }
    else {
      xmlList xl;
      if (xo)
        xo.getObjects(xl);

      xmlList::const_iterator it;

      for (it=xl.begin(); it != xl.end(); ++it) {
        if (it->is("Competitor")){
          int ClubId=it->getObjectInt("ClubId");
          if (addXMLCompetitorDB(*it, ClubId))
            personCount++;
        }
      }
    }

    gdibase.addStringUT(0, lang.tl("Antal importerade: ") + itos(personCount));
    gdibase.refresh();

    setProperty("DatabaseUpdate", getRelativeDay());
  }

  saveRunnerDatabase("database", true);

  if (HasDBConnection) {
    gdibase.addString("", 0, "Uppdaterar serverns databas...");
    gdibase.refresh();

    //msUploadRunnerDB(this);

    OpFailStatus stat = (OpFailStatus)msUploadRunnerDB(this);

    if (stat == opStatusFail) {
      char bf[256];
      msGetErrorState(bf);
      string error = string("Kunde inte ladda upp l�pardatabasen (X).#") + bf;
      throw meosException(error);
    }
    else if (stat == opStatusWarning) {
      char bf[256];
      msGetErrorState(bf);
      gdibase.addInfoBox("", string("Kunde inte ladda upp l�pardatabasen (X).#") + bf, 5000);
    }

    gdibase.addString("", 0, "Klart");
    gdibase.refresh();
  }
}


/*
 <Class lowAge="1" highAge="18" sex="K" actualForRanking="Y">
    <ClassId type="nat" idManager="SWE">6</ClassId>
    <Name>D18 Elit</Name>
    <ClassShortName>D18 Elit</ClassShortName>
    <ClassTypeId>E</ClassTypeId>
    <SubstituteClass>
      <ClassId> 600 </ClassId>
    </SubstituteClass>
    <NotQualifiedSubstituteClass>
      <ClassId> 600 </ClassId>
    </NotQualifiedSubstituteClass>
    <EntryFee>
      <EntryFeeId> A </EntryFeeId>
      <Name> Adult </Name>
      <Amount currency="SEK"> 65 </Amount>
    </EntryFee>
    <ModifyDate>
      <Date> 2001-10-18 </Date>
    </ModifyDate>
  </Class>
*/

bool oEvent::addXMLEvent(const xmlobject &xevent)
{
  if (!xevent)
    return false;

  int id = xevent.getObjectInt("EventId");
  string name;
  xevent.getObjectString("Name", name);

  xmlobject date = xevent.getObject("StartDate");

  if (id>0)
    setExtIdentifier(id);

  setName(name);

  if (date) {
    string dateStr;
    date.getObjectString("Date", dateStr);
    setDate(dateStr);
  }

  synchronize();
  return true;
}

/*
<Course>
  <CourseName> Bana 02 </CourseName>
  <CourseId> 1 </CourseId>
  <ClassShortName> D21E </ClassShortName>
  <CourseVariation>
    <CourseVariationId> 0 </CourseVariationId>
    <CourseLength> 8500 </CourseLength>
    <CourseClimb> 0 </CourseClimb>
    <StartPointCode> S1 </StartPointCode>

    <CourseControl>
    <Sequence> 1 </Sequence>
    <ControlCode> 104 </ControlCode>
    <LegLength> 310 </LegLength>
    </CourseControl>

    <CourseControl>
    <Sequence> 2 </Sequence>
    <ControlCode> 102 </ControlCode>
    <LegLength> 360 </LegLength>
    </CourseControl>

    <FinishPointCode> M1 </FinishPointCode>
    <DistanceToFinish> 157 </DistanceToFinish>
  </CourseVariation>
</Course>

*/
int getLength(const xmlobject &xlen) {
  if (xlen.isnull())
    return 0;
  return xlen.getInt();
}

int getStartIndex(int sn) {
  return sn + 211100;
}

int getFinishIndex(int sn) {
  return sn + 311100;
}

string getStartName(const string &start) {
  int num = getNumberSuffix(start);
  if (num == 0 && start.length()>0)
    num = int(start[start.length()-1])-'0';
  if (num > 0 && num < 10)
    return lang.tl("Start ") + itos(num);
  else if (start.length() == 1)
    return lang.tl("Start");
  else
    return start;
}
/*
<Control>
 <ControlCode> 120 </ControlCode>
 <MapPosition x="-1349.6" y="862.7"/>
</Control>
*/
bool oEvent::addXMLControl(const xmlobject &xcontrol, int type)
{
  // type: 0 control, 1 start, 2 finish
  if (!xcontrol)
    return false;

  xmlobject pos = xcontrol.getObject("MapPosition");

  int xp = 0, yp = 0;
  if (pos) {
    string x,y;
    pos.getObjectString("x", x);
    pos.getObjectString("y", y);
    xp = int(10.0 * atof(x.c_str()));
    yp = int(10.0 * atof(y.c_str()));
  }

  if (type == 0) {
    int code = xcontrol.getObjectInt("ControlCode");
    if (code>=30 && code<1024) {
      pControl pc = getControl(code, true);
      pc->getDI().setInt("xpos", xp);
      pc->getDI().setInt("ypos", yp);
      pc->synchronize();
    }
  }
  else if (type == 1) {
    string start;
    xcontrol.getObjectString("StartPointCode", start);
    start = getStartName(trim(start));
    int num = getNumberSuffix(start);
    if (num == 0 && start.length()>0)
      num = int(start[start.length()-1])-'0';
    pControl pc = getControl(getStartIndex(num), true);
    pc->setNumbers("");
    pc->setName(start);
    pc->setStatus(oControl::StatusStart);
    pc->getDI().setInt("xpos", xp);
    pc->getDI().setInt("ypos", yp);
  }
  else if (type == 2) {
    string finish;
    xcontrol.getObjectString("FinishPointCode", finish);
    finish = trim(finish);
    int num = getNumberSuffix(finish);
    if (num == 0 && finish.length()>0)
      num = int(finish[finish.length()-1])-'0';
    if (num > 0)
      finish = lang.tl("M�l ") + itos(num);
    pControl pc = getControl(getFinishIndex(num), true);
    pc->setNumbers("");
    pc->setName(finish);
    pc->setStatus(oControl::StatusFinish);
    pc->getDI().setInt("xpos", xp);
    pc->getDI().setInt("ypos", yp);
  }

  return true;
}

bool oEvent::addXMLCourse(const xmlobject &xcrs, bool addClasses)
{
  if (!xcrs)
    return false;

  int cid = xcrs.getObjectInt("CourseId");

  string name;
  xcrs.getObjectString("CourseName", name);
  name = trim(name);
  vector<string> cls;
  xmlList obj;
  xcrs.getObjects("ClassShortName", obj);
  for (size_t k = 0; k<obj.size(); k++) {
    string c;
    obj[k].getObjectString(0, c);
    cls.push_back(trim(c));
  }

  //int clsId = xevent.getObjectInt("ClassId");
  vector<pCourse> courses;
  xcrs.getObjects("CourseVariation", obj);
  for (size_t k = 0; k<obj.size(); k++) {
    int variationId = obj[k].getObjectInt("CourseVariationId");
    string varName;
    obj[k].getObjectString("Name", varName);
    //int len = obj[k].getObjectInt("CourseLength");
    xmlobject lenObj = obj[k].getObject("CourseLength");
    int len = getLength(lenObj);
    int climb = getLength(obj[k].getObject("CourseClimb"));
    string start;
    obj[k].getObjectString("StartPointCode", start);
    start = getStartName(trim(start));

    xmlList ctrl;
    obj[k].getObjects("CourseControl", ctrl);
    vector<int> ctrlCode(ctrl.size());
    vector<int> legLen(ctrl.size());
    for (size_t i = 0; i<ctrl.size(); i++) {
      unsigned seq = ctrl[i].getObjectInt("Sequence");
      int index = i;
      if (seq>0 && seq<=ctrl.size())
        index = seq-1;

      ctrlCode[index] = ctrl[i].getObjectInt("ControlCode");
      legLen[index] = getLength(ctrl[i].getObject("LegLength"));
    }

    legLen.push_back(getLength(obj[k].getObject("DistanceToFinish")));
    int actualId = ((cid+1)*100) + variationId;
    string cname=name;
    if (!varName.empty())
      cname += " " + varName;
    else if (obj.size() > 1)
      cname += " " + itos(k+1);

    pCourse pc = 0;
    if (cid > 0)
      pc = getCourseCreate(actualId);
    else {
      pc = getCourse(cname);
      if (pc == 0)
        pc = addCourse(cname);
    }

    pc->setName(cname);
    pc->setLength(len);
    pc->importControls("", false);
    for (size_t i = 0; i<ctrlCode.size(); i++) {
      if (ctrlCode[i]>30 && ctrlCode[i]<1000)
        pc->addControl(ctrlCode[i]);
    }
    if (pc->getNumControls() + 1 == legLen.size())
      pc->setLegLengths(legLen);
    pc->getDI().setInt("Climb", climb);
    pc->setStart(start, true);
    pc->synchronize();

    string finish;
    obj[k].getObjectString("FinishPointCode", finish);
    finish = trim(finish);

    pc->synchronize();

    courses.push_back(pc);
  }

  if (addClasses) {
    for (size_t k = 0; k<cls.size(); k++) {
      pClass pCls = getClassCreate(0, cls[k]);
      if (pCls) {
        if (courses.size()==1) {
          if (pCls->getNumStages()==0) {
            pCls->setCourse(courses[0]);
          }
          else {
            for (size_t i = 0; i<pCls->getNumStages(); i++)
              pCls->addStageCourse(i, courses[0]->getId());
          }
        }
        else {
          if (courses.size() == pCls->getNumStages()) {
            for (size_t i = 0; i<courses.size(); i++)
              pCls->addStageCourse(i, courses[i]->getId());
          }
          else {
            for (size_t i = 0; i<courses.size(); i++)
              pCls->addStageCourse(0, courses[i]->getId());
          }
        }
      }
    }
  }

  return true;
}


bool oEvent::addXMLClass(const xmlobject &xclass)
{
  if (!xclass)
    return false;

  int classid=xclass.getObjectInt("ClassId");
  string name, shortName;
  xclass.getObjectString("Name", name);
  xclass.getObjectString("ClassShortName", shortName);

  if (!shortName.empty())
    name = shortName;

  pClass pc=0;

  if (classid) {
    pc = getClass(classid);

    if (!pc) {
      oClass c(this, classid);
      pc = addClass(c);
    }
  }
  else
    pc = addClass(name);

  pc->setName(name);
  oDataInterface DI=pc->getDI();

  string tmp;
  DI.setInt("LowAge", xclass.getObjectInt("lowAge"));
  DI.setInt("HighAge", xclass.getObjectInt("highAge"));
  DI.setString("Sex", xclass.getObjectString("sex", tmp));
  DI.setString("ClassType", xclass.getObjectString("ClassTypeId", tmp));

  xmlList xFee;
  xclass.getObjects("EntryFee", xFee);
  vector<int> fee;
  for (size_t k = 0; k<xFee.size(); k++) {
    xmlobject xamount = xFee[k].getObject("Amount");
    if (xamount) {
      double f = atof(xamount.get());
      string cur = xamount.getAttrib("currency").get();
      if (f>0) {
        fee.push_back(oe->interpretCurrency(f, cur));
      }
    }
  }

  // XXX Eventor studpid hack
  if (fee.size() == 2 && fee[1]<fee[0] && fee[1]==50)
    fee[1] = ( 3 * fee[0] ) / 2;

  if (!fee.empty()) {
    sort(fee.begin(), fee.end());
    if (fee.size() == 1) {
      DI.setInt("ClassFee", fee[0]);
      DI.setInt("HighClassFee", fee[0]);
    }
    else {
      DI.setInt("ClassFee", fee[0]);
      DI.setInt("HighClassFee", fee[1]);
    }
  }

  pc->synchronize();
  return true;
}


bool oEvent::addXMLClub(const xmlobject &xclub, bool savetoDB)
{
  if (!xclub)
    return false;

  int clubid=xclub.getObjectInt("ClubId");
  string Name, shortName;
  xclub.getObjectString("Name", Name);
  xclub.getObjectString("ShortName", shortName);

  if (!shortName.empty() && shortName.length() < Name.length())
    swap(Name, shortName);

  int district = xclub.getObjectInt("OrganisationId");

  if (Name.length()==0 || !IsCharAlphaNumeric(Name[0]))
    return false;

  xmlobject address=xclub.getObject("Address");

  string str;
  string co;

  if (address) {
    address.getObjectString("street", str).length();
    address.getObjectString("careOf", co).length();
  }

  pClub pc=0;

  if ( !savetoDB ) {
    if (clubid)
      pc = getClubCreate(clubid, Name);

    if (!pc) return false;
  }
  else {
    pc = new oClub(this);
    pc->Id = clubid;
  }

  pc->setName(Name);

  pc->setExtIdentifier(clubid);

  oDataInterface DI=pc->getDI();

  string tmp;
  DI.setString("CareOf", co);
  DI.setString("Street", str);
  if (address) {
    DI.setString("City", address.getObjectString("city", tmp));
    DI.setString("ZIP", address.getObjectString("zipCode", tmp));
  }
  DI.setInt("District", district);

  xmlobject tele=xclub.getObject("Tele");

  if (tele){
    DI.setString("EMail", tele.getObjectString("mailAddress", tmp));
    DI.setString("Phone", tele.getObjectString("phoneNumber", tmp));
  }

  xmlobject country=xclub.getObject("Country");

  if (country) {
    xmlobject natId = country.getObject("CountryId");
    char national[4];
    if (natId)
      DI.setString("Nationality", natId.getObjectString("value", national, 4));
  }

  if (savetoDB) {
    runnerDB->importClub(*pc, false);
    delete pc;
  }
  else {
    pc->synchronize();
  }

  return true;
}


bool oEvent::addXMLRank(const xmlobject &xrank, map<__int64, int> &externIdToRunnerId)
{
  if (!xrank)
    return false;

  xmlobject person;//xrank->getObject("Person");
  xmlobject club;//xrank->getObject("Club");
  xmlobject rank;
  xmlobject vrank;

  xmlList x;
  xrank.getObjects(x);

  xmlList::const_iterator cit=x.begin();

  string tmp;
  while(cit!=x.end()){

    if (cit->is("Person"))
      person=*cit;
    else if (cit->is("Club"))
      club=*cit;
    else if (cit->is("Rank")){
      if (cit->getObjectString("Name", tmp)=="Swedish Ranking List")
        rank=*cit;
      else if (cit->getObjectString("Name", tmp)=="Swedish Vacancy List")
        vrank=*cit;
    }
    ++cit;
  }

  if (!person) return false;

  string pid;
  person.getObjectString("PersonId", pid);
  const __int64 extId = oBase::converExtIdentifierString(pid);
  int id = oBase::idFromExtId(extId);
  if (externIdToRunnerId.count(extId))
    id = externIdToRunnerId[extId];

  pRunner r = getRunner(id, 0);
  
  if (!r){
    xmlobject pname = person.getObject("PersonName");

    if (!pname) return false;

    string given, family;
    string name=getFirst(pname.getObjectString("Given", given), 2)+" "+pname.getObjectString("Family", family);

    if (!club)
      r=getRunnerByName(name);
    else {
      string cn, cns;
      club.getObjectString("ShortName", cns);
      club.getObjectString("Name", cn);

      if (cns.empty())
        cns = cn;

      if (!cn.empty() && cn.length()<cns.length())
        swap(cn, cns);

      r=getRunnerByName(name, cns);
    }
  }

  if (!r) return false; //No runner here!

  oDataInterface DI=r->getDI();

  if (rank)
    DI.setInt("Rank", rank.getObjectInt("RankPosition"));

//  if (vrank)
//    DI.setInt("VacRank", vrank.getObjectInt("RankPosition"));

  r->synchronize();

  return true;
}

void oEvent::exportIOFEventList(xmlparser &xml)
{
  xml.startTag("EventList");
  xml.write("IOFVersion", "version", "2.0.3");

  exportIOFEvent(xml);

  xml.endTag(); //EventList
}

void oEvent::exportIOFEvent(xmlparser &xml)
{
  // (IndSingleDay|IndMultiDay|teamSingleDay|teamMultiDay|relay)
  xml.startTag("Event", "eventForm", "IndSingleDay");

  xml.write("EventId", getExtIdentifierString());
  xml.write("Name", getName());

  xml.write("EventClassificationId", "type", "other", "MeOS");

  {
    xml.startTag("StartDate");
    xml.write("Date", "dateFormat", "YYYY-MM-DD", getDate());
    xml.write("Clock", "clockFormat", "HH:MM:SS", getZeroTime());
    xml.endTag(); // StartDate
  }

  string url = getDCI().getString("Homepage");
  if (!url.empty())
    xml.write("WebURL", url);

  string account = getDCI().getString("Account");
  if (!account.empty())
    xml.write("Account", "type", "other", account);

  xml.endTag(); //Event
}

void oEvent::exportIOFClass(xmlparser &xml)
{
  xml.startTag("ClassData");

  set<string> cls;
  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    if (!it->getType().empty())
      cls.insert(it->getType());
  }

  int id = 1;
  map<string, int> idMap;
  for (set<string>::iterator it = cls.begin(); it != cls.end(); ++it) {
    xml.startTag("ClassType");
    idMap[*it] = id;
    xml.write("ClassTypeId", id++);
    xml.write("Name", *it);
    xml.endTag();
  }

  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    vector<string> pv;

    pClass pc = &*it;
    int low = 0;
    int high = 0;
    pc->getAgeLimit(low, high);

    if (low>0) {
      pv.push_back("lowAge");
      pv.push_back(itos(low));
    }
    if (high>0) {
      pv.push_back("highAge");
      pv.push_back(itos(high));
    }

    string sex = encodeSex(pc->getSex());
    if (sex.empty())
      sex = "B";

    pv.push_back("sex");
    pv.push_back(sex);

    if (pc->getNumStages()>1) {
      pv.push_back("numberInTeam");
      pv.push_back(itos(pc->getNumStages()));
    }

    if (pc->getClassType() == oClassRelay) {
      pv.push_back("teamEntry");
      pv.push_back("Y");
    }

    if (pc->getNoTiming()) {
      pv.push_back("timePresentation");
      pv.push_back("N");
    }

    xml.startTag("Class", pv);

    xml.write("ClassId", itos(pc->getId()));

    xml.write("ClassShortName", pc->getName());

    if (!pc->getType().empty())
      xml.write("ClassTypeId", itos(idMap[pc->getType()]));

    xml.endTag();
  }
  xml.endTag();
}

void oEvent::exportIOFClublist(xmlparser &xml)
{
  xml.startTag("ClubList");
  xml.write("IOFVersion", "version", "2.0.3");

  for (oClubList::iterator it = Clubs.begin(); it!=Clubs.end(); ++it) {
    it->exportIOFClub(xml, true);
  }

  xml.endTag();
}

void cWrite(xmlparser &xml, const char *tag, const string &value) {
  if (!value.empty()) {
    xml.write(tag, value);
  }
}

void oClub::exportClubOrId(xmlparser &xml) const
{
  if (getExtIdentifier() != 0)
    xml.write("ClubId", getExtIdentifierString());
  else {
    exportIOFClub(xml, true);
  }
}

void oClub::exportIOFClub(xmlparser &xml, bool compact) const
{
  xml.startTag("Club");
  if (getExtIdentifier() != 0)
    xml.write("ClubId", getExtIdentifierString());
  else
    xml.write("ClubId");

  xml.write("ShortName", getName());

  if (compact) {
    xml.endTag();
    return;
  }

  string country = getDCI().getString("Nationality");
  if (!country.empty())
    xml.write("CountryId", "value", country);

  int district = getDCI().getInt("District");
  if (district>0)
    xml.write("OrganisationId", itos(district));

  vector<string> pv;

  // Address
  string co = getDCI().getString("CareOf");
  string street = getDCI().getString("Street");
  string city = getDCI().getString("City");
  string zip = getDCI().getString("ZIP");

  if (!co.empty()) {
    pv.push_back("careOf");
    pv.push_back(co);
  }
  if (!street.empty()) {
    pv.push_back("street");
    pv.push_back(street);
  }
  if (!city.empty()) {
    pv.push_back("city");
    pv.push_back(city);
  }
  if (!zip.empty()) {
    pv.push_back("zipCode");
    pv.push_back(zip);
  }
  if (!pv.empty()) {
    xml.startTag("Address", pv);
    xml.endTag();
  }
  pv.clear();

  //Tele
  string mail = getDCI().getString("EMail");
  string phone = getDCI().getString("Phone");

  if (!mail.empty()) {
    pv.push_back("mailAddress");
    pv.push_back(mail);
  }
  if (!phone.empty()) {
    pv.push_back("phoneNumber");
    pv.push_back(phone);
  }
  if (!pv.empty()) {
    xml.startTag("Tele", pv);
    xml.endTag();
  }

  //Club
  xml.endTag();
}

void oEvent::exportIOFStartlist(xmlparser &xml)
{
  xml.startTag("StartList");
  xml.write("IOFVersion", "version", "2.0.3");

  exportIOFEvent(xml);

  for (oClassList::iterator it = Classes.begin(); it!=Classes.end(); ++it) {
    xml.startTag("ClassStart");
    //xml.write("ClassId", itos(it->getId()));
    xml.write("ClassShortName", it->getName());
    it->exportIOFStart(xml);
    xml.endTag();
  }
  xml.endTag();
}

void oClass::exportIOFStart(xmlparser &xml) {
  bool useEventor = oe->getPropertyInt("UseEventor", 0) == 1;

  if (getClassType() == oClassIndividual || getClassType() == oClassIndividRelay) {
    for (oRunnerList::iterator it = oe->Runners.begin(); it!=oe->Runners.end(); ++it) {
      if (it->getClassId() != getId() || it->isRemoved())
        continue;

      xml.startTag("PersonStart");

      it->exportIOFRunner(xml, true);

      if (it->getClubId()>0)
        it->Club->exportClubOrId(xml);

      int rank = it->getDCI().getInt("Rank");
      if (rank>0) {
        //Ranking
        xml.startTag("Rank");
        xml.write("Name", "MeOS");
        xml.write("RankPosition", itos(rank));
        xml.write("RankValue", itos(rank));
        xml.endTag();
      }

      int multi = it->getNumMulti();
      if (multi==0)
        it->exportIOFStart(xml);
      else {
        xml.startTag("RaceStart");
        xml.write("EventRaceId", "1");
        it->exportIOFStart(xml);
        xml.endTag();
        for (int k = 0; k < multi; k++) {
          pRunner r = it->getMultiRunner(k+1);
          if (r) {
            xml.startTag("RaceStart");
            xml.write("EventRaceId", itos(k+2));
            r->exportIOFStart(xml);
            xml.endTag();
          }
        }
      }
      xml.endTag();
    }
  }
  else if (getClassType() == oClassRelay || getClassType() == oClassPatrol) {

    // A bug in Eventor / OLA results in an internal error if a patrol has a team name.
    // Set writeTeamName to true (or remove) when this bug is fixed.
    bool writeTeamName = !useEventor || getClassType() != oClassPatrol;

    for (oTeamList::iterator it = oe->Teams.begin(); it!=oe->Teams.end(); ++it) {
      if (it->getClassId() != getId() || it->isRemoved())
        continue;

      xml.startTag("TeamStart");

      if (writeTeamName)
        xml.write("TeamName", it->getName());

      string nat = it->getDCI().getString("Nationality");
      if (!nat.empty())
        xml.write("CountryId", "value", nat);

      for (size_t k=0; k<it->Runners.size(); k++) {
        if (it->Runners[k]) {
          xml.startTag("PersonStart");

          pRunner parent = it->Runners[k]->getMultiRunner(0);
          if (parent != 0)
            parent->exportIOFRunner(xml, true);

          if (it->Runners[k]->getClubId()>0) {
            it->Runners[k]->Club->exportClubOrId(xml);
          }
          else if (it->getClubId()>0) {
            it->Club->exportClubOrId(xml);
          }

          it->Runners[k]->exportIOFStart(xml);
          xml.endTag();
        }
      }

      xml.endTag();
    }
  }
}

void oRunner::exportIOFStart(xmlparser &xml)
{
  xml.startTag("Start");
  int sno = getStartNo();
  if (sno>0)
    xml.write("StartNumber", itos(sno));

  xml.startTag("StartTime");
  xml.write("Clock", "clockFormat", "HH:MM:SS",
              formatTimeIOF(getStartTime(), oe->ZeroTime));
  xml.endTag();

  string bib = getBib();
  if (!bib.empty())
    xml.write("BibNumber", bib);

  if (getCardNo() > 0)
    xml.write("CCardId", itos(getCardNo()));

  int len = 0;
  if (getCourse(false)) {
    len = getCourse(false)->getLength();
    if (len>0)
      xml.write("CourseLength", "unit", "m", itos(len));

    string start = getCourse(false)->getStart();
    if (!start.empty())
      xml.write("StartId", max(1, getNumberSuffix(start)));
  }

  if (tInTeam) {
    xml.write("TeamSequence", itos(tLeg+1));
  }

  xml.endTag();
}

void oRunner::exportIOFRunner(xmlparser &xml, bool compact)
{
  string sex = encodeSex(getSex());

  if (sex.length()==1)
    xml.startTag("Person", "sex", sex);
  else
    xml.startTag("Person");

  if (getExtIdentifier() != 0)
    xml.write("PersonId", getExtIdentifierString());
  else
    xml.write("PersonId");

  xml.startTag("PersonName");
  xml.write("Family", getFamilyName());
  xml.write("Given", "sequence", "1", getGivenName());
  xml.endTag();

  int year = getBirthYear();

  if (year>0 && !compact) {
    xml.startTag("BirthDate");
    xml.write("Date", "dateFormat", "YYYY", itos(extendYear(year)));
    xml.endTag();
  }

  xml.endTag();
}

void oEvent::exportIOFResults(xmlparser &xml, bool selfContained, const set<int> &classes, int leg, bool oldStylePatol)
{
  vector<SplitData> dummy;
  xml.startTag("ResultList");

  xml.write("IOFVersion", "version", "2.0.3");

  exportIOFEvent(xml);

  bool ClassStarted=false;
  int Id=-1;
  bool skipClass=false;
  if (oldStylePatol) {
    // OLD STYLE PATROL EXPORT
    for (oTeamList::iterator it=Teams.begin(); it != Teams.end(); ++it) {
      if (it->isRemoved())
        continue;
      if (it->Runners.size()>=2 && it->Runners[0] && it->Runners[1]) {
        if (it->getClassId()!=Id) {
          if (ClassStarted) xml.endTag();

          if (!it->Class || it->Class->getClassType()!=oClassPatrol) {
            skipClass=true;
            ClassStarted=false;
            continue;
          }

          if ((!classes.empty() && classes.count(it->getClassId()) == 0) || leg != -1) {
            skipClass=true;
            ClassStarted=false;
            continue;
          }

          skipClass=false;
          xml.startTag("ClassResult");
          ClassStarted=true;
          Id=it->getClassId();

          xml.write("ClassShortName", it->getClass());
        }

        if (skipClass)
          continue;

        xml.startTag("PersonResult");
          it->Runners[0]->exportIOFRunner(xml, true);

          xml.startTag("Club");
            xml.write("ClubId", 0);
            xml.write("ShortName", it->Runners[1]->getName());
            xml.write("CountryId", "value", it->getDI().getString("Nationality"));
          xml.endTag();

          xml.startTag("Result");
            xml.startTag("StartTime");
              xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getStartTime(), ZeroTime));
            xml.endTag();
            xml.startTag("FinishTime");
              xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getLegFinishTime(-1), ZeroTime));
            xml.endTag();

            xml.write("Time", "timeFormat", "HH:MM:SS", formatTimeIOF(it->getLegRunningTime(-1, false), 0));
            xml.write("ResultPosition", it->getLegPlaceS(-1, false));

            xml.write("CompetitorStatus", "value", it->Runners[0]->getIOFStatusS());

            const vector<SplitData> &sp=it->Runners[0]->getSplitTimes(true);

            pCourse pc=it->Runners[0]->getCourse(false);
            if (pc) xml.write("CourseLength", "unit", "m", pc->getLengthS());

            pCourse pcourse=pc;
            if (pcourse && it->getLegStatus(-1, false)>0 && it->getLegStatus(-1, false)!=StatusDNS) {
              int no = 1;
              bool hasRogaining = pcourse->hasRogaining();
              int startIx = pcourse->useFirstAsStart() ? 1 : 0;
              int endIx = pcourse->useLastAsFinish() ? pcourse->nControls - 1 : pcourse->nControls;
              for (int k=startIx;k<endIx;k++) {
                if (pcourse->Controls[k]->isRogaining(hasRogaining))
                  continue;
                xml.startTag("SplitTime", "sequence", itos(no++));
                xml.write("ControlCode", pcourse->Controls[k]->getFirstNumber());
                if (unsigned(k)<sp.size() && sp[k].time>0)
                  xml.write("Time", "clockFormat", "HH:MM:SS", getAbsTime((sp[k].time-it->tStartTime)-ZeroTime));
                else
                  xml.write("Time", "--:--:--");

                xml.endTag();
              }
            }
          xml.endTag();
        xml.endTag();
      }
    }

    if (ClassStarted) {
      xml.endTag();
      ClassStarted = false;
    }
  }
  // OldStylePatrol

  if (leg == -1)
    exportTeamSplits(xml, classes, oldStylePatol);

  skipClass=false;
  Id=-1;

  for (oRunnerList::iterator it=Runners.begin();
        it != Runners.end(); ++it) {

    if (it->isRemoved() || (leg != -1 && it->tLeg != leg) || it->isVacant())
      continue;

    if (it->getClassId()!=Id) {
      if (ClassStarted) xml.endTag();

      if (!it->Class) {
        skipClass=true;
        ClassStarted=false;
        continue;
      }

      ClassType ct = it->Class->getClassType();

      if (leg == -1 && (ct == oClassPatrol || ct ==oClassRelay || ct == oClassIndividRelay) ) {
        skipClass=true;
        ClassStarted=false;
        continue;
      }

      if ( (!classes.empty() && classes.count(it->getClassId()) == 0) ) {
        skipClass=true;
        ClassStarted=false;
        continue;
      }

      xml.startTag("ClassResult");
      ClassStarted=true;
      skipClass=false;
      Id=it->getClassId();

      xml.write("ClassShortName", it->getClass());
    }

    if (skipClass)
      continue;

    xml.startTag("PersonResult");

      it->exportIOFRunner(xml, true);

      if (it->Club)
        it->Club->exportIOFClub(xml, true);

      xml.startTag("Result");
        xml.startTag("CCard");
          xml.write("CCardId", it->getCardNo());
        xml.endTag();
        xml.startTag("StartTime");
        xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getStartTime(), ZeroTime));
        xml.endTag();
        xml.startTag("FinishTime");
        xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getFinishTimeAdjusted(), ZeroTime));
        xml.endTag();

        xml.write("Time", "timeFormat", "HH:MM:SS", formatTimeIOF(it->getRunningTime(),0));
        xml.write("ResultPosition", it->getPlaceS());

        xml.write("CompetitorStatus", "value", it->getIOFStatusS());

        const vector<SplitData> &sp=it->getSplitTimes(true);
        pCourse pc=it->getCourse(false);
        if (pc) xml.write("CourseLength", "unit", "m", pc->getLengthS());

        pCourse pcourse=it->getCourse(true);
        if (pcourse && it->getStatus()>0 && it->getStatus()!=StatusDNS
          && it->getStatus()!=StatusNotCompetiting) {
          bool hasRogaining = pcourse->hasRogaining();
          int no = 1;
          int startIx = pcourse->useFirstAsStart() ? 1 : 0;
          int endIx = pcourse->useLastAsFinish() ? pcourse->nControls - 1 : pcourse->nControls;
          for (int k=startIx;k<endIx;k++) {
            if (pcourse->Controls[k]->isRogaining(hasRogaining))
              continue;
            xml.startTag("SplitTime", "sequence", itos(no++));
            xml.write("ControlCode", pcourse->Controls[k]->getFirstNumber());
            if (unsigned(k)<sp.size() && sp[k].time>0)
              xml.write("Time", "timeFormat", "HH:MM:SS", getAbsTime((sp[k].time-it->tStartTime)-ZeroTime));
            else
              xml.write("Time", "--:--:--");

            xml.endTag();
          }
        }
      xml.endTag();
    xml.endTag();
  }

  if (ClassStarted) {
    xml.endTag();
    ClassStarted = false;
  }

  xml.endTag();
}

void oEvent::exportTeamSplits(xmlparser &xml, const set<int> &classes, bool oldStylePatrol)
{
  vector<SplitData> dummy;
  bool ClassStarted=false;
  int Id=-1;
  bool skipClass=false;

  sortTeams(ClassResult, -1, true);
  for(oTeamList::iterator it=Teams.begin(); it != Teams.end(); ++it) {
    if (it->isRemoved())
      continue;
    if (it->getClassId()!=Id) {
      if (ClassStarted) {
        xml.endTag();
        ClassStarted = false;
      }

      if (!it->Class) {
        skipClass=true;
        continue;
      }

      ClassType ct = it->Class->getClassType();

      if (oldStylePatrol && ct == oClassPatrol) {
        skipClass=true;
        continue;
      }

      if (ct != oClassRelay && ct != oClassIndividRelay && ct != oClassPatrol) {
        skipClass=true;
        continue;
      }

      if (!classes.empty() && classes.count(it->getClassId()) == 0) {
        skipClass=true;
        continue;
      }

      skipClass=false;
      xml.startTag("ClassResult");
      ClassStarted=true;
      Id=it->getClassId();

      xml.write("ClassShortName", it->getClass());
    }

    if (skipClass)
      continue;

    xml.startTag("TeamResult"); {
      /*
    <TeamName>Sundsvalls OK</TeamName>
    <BibNumber></BibNumber>
    <StartTime>
     <Clock clockFormat="HH:MM:SS">10:00:00</Clock>
    </StartTime>
    <FinishTime>
     <Clock clockFormat="HH:MM:SS">11:45:52</Clock>
    </FinishTime>
    <Time>
     <Time timeFormat="HH:MM:SS">01:45:52</Time>
    </Time>
    <ResultPosition>1</ResultPosition>
    <TeamStatus value="OK"></TeamStatus>
      */
      string nat = it->getDCI().getString("Nationality");
      if (!nat.empty())
        xml.write("CountryId", "value", nat);

      xml.write("TeamName", it->getName());
      xml.write("BibNumber", it->getStartNo());

      xml.startTag("StartTime");
        xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getStartTime(), ZeroTime));
      xml.endTag();
      xml.startTag("FinishTime");
      xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(it->getFinishTimeAdjusted(), ZeroTime));
      xml.endTag();

      xml.write("Time", "timeFormat", "HH:MM:SS", formatTimeIOF(it->getRunningTime(), 0));
      xml.write("ResultPosition", it->getPlaceS());
      xml.write("TeamStatus", "value", it->getIOFStatusS());

      for (size_t k=0;k<it->Runners.size();k++) {
        if (!it->Runners[k])
          continue;
        pRunner r=it->Runners[k];

        xml.startTag("PersonResult"); {

          r->exportIOFRunner(xml, true);

          if (r->Club)
            r->Club->exportIOFClub(xml, true);

          xml.startTag("Result"); {
            xml.write("TeamSequence", k+1);
            xml.startTag("StartTime");
              xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(r->getStartTime(), ZeroTime));
            xml.endTag();
            xml.startTag("FinishTime");
            xml.write("Clock", "clockFormat", "HH:MM:SS", formatTimeIOF(r->getFinishTimeAdjusted(), ZeroTime));
            xml.endTag();

            xml.write("Time", "timeFormat", "HH:MM:SS", formatTimeIOF(r->getRunningTime(), 0));
            xml.write("ResultPosition", r->getPlaceS());

            xml.write("CompetitorStatus", "value", r->getIOFStatusS());

            const vector<SplitData> &sp = r->getSplitTimes(true);

            pCourse pc=r->getCourse(false);

            if (pc) {
              xml.startTag("CourseVariation");
              xml.write("CourseVariationId", pc->getId());
              xml.write("CourseLength", "unit", "m", pc->getLengthS());
              xml.endTag();
            }
            pCourse pcourse=pc;
            if (pcourse && r->getStatus()>0 && r->getStatus()!=StatusDNS
                  && r->getStatus()!=StatusNotCompetiting) {
              int no = 1;
              bool hasRogaining = pcourse->hasRogaining();
              int startIx = pcourse->useFirstAsStart() ? 1 : 0;
              int endIx = pcourse->useLastAsFinish() ? pcourse->nControls - 1 : pcourse->nControls;
              for (int k=startIx;k<endIx;k++) {
                if (pcourse->Controls[k]->isRogaining(hasRogaining))
                  continue;
                xml.startTag("SplitTime", "sequence", itos(no++));
                xml.write("ControlCode", pcourse->Controls[k]->getFirstNumber());
                if (unsigned(k)<sp.size() && sp[k].time>0)
                  xml.write("Time", "clockFormat", "HH:MM:SS", getAbsTime((sp[k].time-r->tStartTime)-ZeroTime));
                else
                  xml.write("Time", "--:--:--");

                xml.endTag();
              } //Loop over splits
            }
          } xml.endTag();
        } xml.endTag();
      } //Loop over team members

    }  xml.endTag();    // Team result
  }

  if (ClassStarted) {
    xml.endTag();
    ClassStarted = false;
  }

}

void oEvent::exportIOFSplits(IOFVersion version, const char *file,
                             bool oldStylePatrolExport, bool useUTC,
                             const set<int> &classes, int leg,
                             bool teamsAsIndividual, bool unrollLoops,
                             bool includeStageInfo, bool forceSplitFee) {
  xmlparser xml(gdibase.getEncoding() == ANSI ? 0 : &gdibase);

  xml.openOutput(file, false);
  oClass::initClassId(*this);
  reEvaluateAll(set<int>(), true);
  if (version != IOF20)
    calculateResults(RTClassCourseResult);
  calculateResults(RTTotalResult);
  calculateResults(RTClassResult);
  calculateTeamResults(true);
  calculateTeamResults(false);

  if (version == IOF20)
    exportIOFResults(xml, true, classes, leg, oldStylePatrolExport);
  else {
    IOF30Interface writer(this, forceSplitFee);
    writer.writeResultList(xml, classes, leg, useUTC, 
                           teamsAsIndividual, unrollLoops, includeStageInfo);
  }

  xml.closeOut();
}

void oEvent::exportIOFStartlist(IOFVersion version, const char *file, bool useUTC,
                                const set<int> &classes, bool teamsAsIndividual,
                                bool includeStageInfo, bool forceSplitFee) {
  xmlparser xml(gdibase.getEncoding() == ANSI ? 0 : &gdibase);
  
  oClass::initClassId(*this);
  xml.openOutput(file, false);

  if (version == IOF20)
    exportIOFStartlist(xml);
  else {
    IOF30Interface writer(this, forceSplitFee);
    writer.writeStartList(xml, classes, useUTC, teamsAsIndividual, includeStageInfo);
  }
  xml.closeOut();
}
