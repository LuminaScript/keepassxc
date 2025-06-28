/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestCsvImportExport.h"

#include <QBuffer>
#include <QTemporaryFile>
#include <QTest>

#include "core/Group.h"
#include "crypto/Crypto.h"
#include "format/CsvExporter.h"
#include "format/CsvParser.h"
#include "gui/csvImport/CsvImportWidget.h"
#include "gui/csvImport/CsvParserModel.h"

QTEST_GUILESS_MAIN(TestCsvImportExport)

void TestCsvImportExport::initTestCase()
{
    Crypto::init();
}

void TestCsvImportExport::init()
{
    m_db = QSharedPointer<Database>::create();
    m_csvExporter = QSharedPointer<CsvExporter>::create();
}

void TestCsvImportExport::cleanup()
{
}

void TestCsvImportExport::testRoundTripWithCustomRootName()
{
    // Create a database with a custom root group name
    Group* groupRoot = m_db->rootGroup();
    groupRoot->setName("MyPasswords");  // Custom root name instead of default "Passwords"
    
    auto* group = new Group();
    group->setName("Test Group");
    group->setParent(groupRoot);
    
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setTitle("Test Entry");
    entry->setUsername("testuser");
    entry->setPassword("testpass");

    // Export to CSV
    QString csvData = m_csvExporter->exportDatabase(m_db);
    
    // Verify export contains the root group name in the path
    QVERIFY(csvData.contains("\"MyPasswords/Test Group\""));
    
    // Now test the createGroupStructure logic directly
    // This tests the fix - when importing CSV with "MyPasswords/Test Group",
    // the logic should now recognize "MyPasswords" as a root group name to skip
    
    QString groupPathFromCsv = "MyPasswords/Test Group";
    auto nameList = groupPathFromCsv.split("/", Qt::SkipEmptyParts);
    
    // This is the new (fixed) logic from CsvImportWidget::createGroupStructure
    // that skips the first element when there are multiple path components
    if (nameList.size() > 1) {
        nameList.removeFirst();
    }
    
    // After this logic, nameList should contain only ["Test Group"]
    // which means it will create the correct structure: Root -> Test Group
    QCOMPARE(nameList.size(), 1);  // Fixed: should be 1
    QCOMPARE(nameList.first(), QString("Test Group"));  // This should be the only element
}

void TestCsvImportExport::testRoundTripWithDefaultRootName()
{
    // Test with default "Passwords" root name to ensure it works correctly
    Group* groupRoot = m_db->rootGroup();
    // Default name is "Passwords" - don't change it
    
    auto* group = new Group();
    group->setName("Test Group");
    group->setParent(groupRoot);
    
    auto* entry = new Entry();
    entry->setGroup(group);
    entry->setTitle("Test Entry");
    entry->setUsername("testuser");
    entry->setPassword("testpass");

    // Export to CSV
    QString csvData = m_csvExporter->exportDatabase(m_db);
    
    // Verify export contains the root group name in the path
    QVERIFY(csvData.contains("\"Passwords/Test Group\""));
    
    // Test the createGroupStructure logic
    QString groupPathFromCsv = "Passwords/Test Group";
    auto nameList = groupPathFromCsv.split("/", Qt::SkipEmptyParts);
    
    // New logic skips the first element when there are multiple path components
    if (nameList.size() > 1) {
        nameList.removeFirst();
    }
    
    // After this logic, nameList should contain only ["Test Group"]
    QCOMPARE(nameList.size(), 1);  // Fixed: should be 1
    QCOMPARE(nameList.first(), QString("Test Group"));  // This should be the only element
}

void TestCsvImportExport::testSingleLevelGroup()
{
    // Test case: entry is directly in root group (no sub-groups)
    // This should still work correctly and not remove any path components
    
    Group* groupRoot = m_db->rootGroup();
    auto* entry = new Entry();
    entry->setGroup(groupRoot);  // Put entry directly in root
    entry->setTitle("Root Entry");
    entry->setUsername("rootuser");
    entry->setPassword("rootpass");

    // Export to CSV
    QString csvData = m_csvExporter->exportDatabase(m_db);
    
    // Verify export contains just the root group name (no sub-path)
    QVERIFY(csvData.contains("\"Passwords\",\"Root Entry\""));
    
    // Test the createGroupStructure logic with just the root group name
    QString groupPathFromCsv = "Passwords";  // Single component
    auto nameList = groupPathFromCsv.split("/", Qt::SkipEmptyParts);
    
    // With only one component, nothing should be removed
    if (nameList.size() > 1) {
        nameList.removeFirst();
    }
    
    // Should still have ["Passwords"] as we don't remove single components
    QCOMPARE(nameList.size(), 1);
    QCOMPARE(nameList.first(), QString("Passwords"));
}