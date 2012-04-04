/*
 * This file is part of qZDL
 * Copyright (C) 2007-2010  Cody Harris
 * 
 * qZDL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <cstring>



using namespace std;
#include <zdlcommon.h>


extern QString chomp(QString in);

/* ZDLConf.cpp
 *    Author: Cody Harris
 *   Purpose: To load in a configuration file, and parse it into sections and lines
 * Rationale: The current implementation of configuration files stores the values
 *            in a system which forgets keys and comments it doesn't know about.
 *            This system caches everything, and makes changes on the fly so that
 *            config files can be written back with keys it doesn't know how to use.
 *      Date: July 29th, 2007
 */

int ZDLConf::readINI(QString file)
{	
	if((mode & ZDLConf::FileRead) != 0){
		reads++;
		QString line;
		/* We allow lines to be outside of any section (ie header comments)
		* We use a global section to read this.  We also keep track of
		* which section we're in.  We do that with a pointer (current)
		*/
		ZDLSection *current = new ZDLSection("");
		current->setSpecial(ZDL_FLAG_NAMELESS);
		sections.push_back(current);
		QFile stream(file);
		stream.open(QIODevice::ReadOnly);
		if (!stream.isOpen()){
			cerr << "Unable to open file \"" << file.toStdString() << "\"" << std::endl;
			return 1;
		}
		while (!stream.atEnd()){
			QByteArray array = stream.readLine();
			QString line(array);
			parse(line, current);
			current = sections.back();
		}
		stream.close();
		
		QFileInfo info(file);
		if(!info.isWritable()){
			mode = mode & ~FileWrite;
		}
		
		return 0;
	}else{
		return 1;
	}
}

int ZDLConf::numberOfSections()
{
	return sections.size();;

}

int ZDLConf::writeINI(QString file)
{
	if((mode & ZDLConf::FileWrite) != 0){
		writes++;
		QFile stream(file);
		stream.open(QIODevice::WriteOnly);
		if (!stream.isOpen()){
			cerr << "Unable to open file \"" << file.toStdString() << "\"" << endl;
			return 1;
		}
		QIODevice *dev = &stream;
		writeStream(dev);
		stream.close();
		return 0;
	}else{
		return 1;
	}
}

int ZDLConf::writeStream(QIODevice *stream){
	if((mode & ZDLConf::FileWrite) != 0){
		for(int i = 0; i < sections.size(); i++){
			sections[i]->streamWrite(stream);
		}
		return 0;
	}else{
		return 1;
	}
}

ZDLConf::ZDLConf(int mode)
{
	this->mode = mode;
	//cout << "New configuration" << endl;
	reads = 0;
	writes = 0;
	//vars = new ZDLVariables(this);
}

/* int ZDLConf::reopen(int mode)
 * Returns: 0 on success, nonzero on failure
 */
int ZDLConf::reopen(int mode){
	this->mode = mode;
	return 0;
}

ZDLConf::~ZDLConf()
{
	//cout << "Configuration deleted." << endl;
	while (sections.size() > 0){
		ZDLSection* section = sections.front();
		sections.pop_front();
		delete section;
	}
	//if (vars)
	//	delete vars;
	//cout << "Deleting configuration children." << endl;

}

void ZDLConf::deleteSection(QString lsection){
	for(int i = 0; i < sections.size(); i++){
		ZDLSection* section = sections[i];
                QString secName = section->getName();
                if (secName == lsection){
                        sections.remove(i);
                        return;
                }
	}
}

void ZDLConf::deleteValue(QString lsection, QString variable){
	if((mode & WriteOnly) != 0){
		writes++;
		for(int i = 0; i < sections.size(); i++){
			ZDLSection* section = sections[i];
                        if (section->getName().compare(lsection, Qt::CaseInsensitive) == 0){
                                section->deleteVariable(variable);

                        }

		}
	}
}

QString ZDLConf::getValue(QString lsection, QString variable, int *status){
	if((mode & ReadOnly) != 0){
		reads++;
		//if (vars){
		//	//cout << "ZDLConf::getValue using variable resolution" << endl;
		//	QString rc = vars->getVariable(lsection, variable, status);
		//	return rc;
		//}else{
			//cout << "ZDLConf::getValue using non-variable resolution" << endl;
			ZDLSection *sect = getSection(lsection);
			if (sect){
				*status = 1;
				return sect->findVariable(variable);
			}
	
		//}
		*status = 0;
	}else{
		*status = 2;
	}
	
	return QString();
}

//char *ZDLConf::getValue(QString lsection, QString variable){
//	int stat;
//	string temp = getValue(lsection, variable, &stat);
//	return (char*)temp.c_str();
//}

ZDLSection *ZDLConf::getSection(QString lsection){
	if((mode & ReadOnly) != 0){
		for(int i = 0; i < sections.size(); i++){
			ZDLSection* section = sections[i];
                        if (section->getName().compare(lsection,Qt::CaseInsensitive) == 0){
                                return section;
                        }
		}
	}
	return NULL;
}

int ZDLConf::hasValue(QString lsection, QString variable){
	if((mode & ReadOnly) != 0){
		reads++;
		//If we actually have a variable resolver, lets use that.
		//if (vars){
		//	int nRc = 0;
		//	return vars->hasVariable(lsection, variable, &nRc);
		//Otherwise, lets look for it ourself.
		//}else{
			for(int i = 0; i < sections.size(); i++){
				ZDLSection* section = sections[i];
                                if (section->getName().compare(lsection, Qt::CaseInsensitive) == 0){
                                        return section->hasVariable(variable);
                                }

			}
		//}
	}
	return false;
}

int ZDLConf::setValue(QString lsection, QString variable, int value)
{
	if((mode & WriteOnly) != 0){
		char szBuffer[256];
		snprintf(szBuffer, 256, "%d", value);
		return setValue(lsection,variable,szBuffer);
	}else{
		return -1;
	}
}

int ZDLConf::setValue(QString lsection, QString variable, QString szBuffer)
{
	if((mode & WriteOnly) != 0){
		QString value = szBuffer;
		
		//Better handing of variables.  Don't overwrite if you don't have to.
		if(hasValue(lsection,variable)){
			int stat;
			QString oldValue = getValue(lsection, variable, &stat);
			if(oldValue == szBuffer){
				return 0;
			}
		}
		
		writes++;
		for(int i = 0; i < sections.size(); i++){
			ZDLSection* section = sections[i];
                        if (section->getName().compare(lsection,Qt::CaseInsensitive) == 0){
                                section->setValue(variable, value);
                                return 0;
                        }

		}
		//In this case, we didn't find the section
		ZDLSection *section = new ZDLSection(lsection);
		sections.push_back(section);
		//cout << "int ZDLConf::setValue("<<lsection<<","<<variable<<","<<szBuffer<<")"<<endl;
		section->setValue(variable, value);
	}
	return 0;
}

void ZDLConf::parse(QString in, ZDLSection* current)
{
	if (in.length() < 1){
		return;
	}
	QString chomped = chomp(in);
	if (chomped[0] == '[' && chomped[chomped.length() - 1] == ']'){
		chomped = chomped.mid(1, chomped.length()-2);
		//This will remove duplicate sections automagically
		ZDLSection *ptr = getSection(chomped);
		if (ptr == NULL){
			current = new ZDLSection(chomped);
			sections.push_back(current);
		}else{
			current = ptr;
		}
	}else{
		current->addLine(chomped);
	}
}

ZDLConf *ZDLConf::clone(){
	ZDLConf *copy = new ZDLConf();
	for(int i = 0; i < sections.size(); i++){
		copy->addSection(sections[i]->clone());
	}
	return copy;
}





