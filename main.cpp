/*
    Author: Jesper Thomschutz 2008, jesper@jespersaur.com

    This file is part of Trippy.

    Trippy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Trippy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Trippy.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "trippy.h"

int main(int argc, char* argv[])
{
  QCoreApplication::setApplicationName("trippy");

  QApplication app(argc, argv);
  Trippy trippy;
  
  // read the command-line arguments - for now we only expect filenames:
  QStringList fileNames;
  for (int i=1; i<argc; i++)
  {
    fileNames << QString::fromLocal8Bit(argv[i]);
  }
  trippy.filesSelected(fileNames);
  
  return app.exec();
}
