"""Keep the Simplified Chinese catalog complete and preserve runtime formatting."""

from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]


def messages(path):
    root = ET.parse(path).getroot()
    entries = {}
    for context in root.findall('context'):
        for message in context.findall('message'):
            key = (context.findtext('name'), message.findtext('source'),
                   message.findtext('comment', ''), message.get('numerus', 'no'))
            entries[key] = message
    return root, entries


class ChineseCatalogTest(unittest.TestCase):
    def test_complete_catalog_and_formatting(self):
        _, english = messages(ROOT / 'translations/immich_en.ts')
        chinese_root, chinese = messages(ROOT / 'translations/immich_zh_CN.ts')
        self.assertEqual(chinese_root.get('language'), 'zh_CN')
        self.assertEqual(chinese_root.get('sourcelanguage'), 'en')
        self.assertEqual(english.keys(), chinese.keys(),
                         'Simplified Chinese must cover every source message')
        for key, message in chinese.items():
            with self.subTest(context=key[0], source=key[1]):
                source = key[1]
                translation = message.find('translation')
                self.assertIsNotNone(translation)
                self.assertNotIn(translation.get('type'),
                                 ('unfinished', 'obsolete', 'vanished'))
                if message.get('numerus') == 'yes':
                    forms = [form.text or ''
                             for form in translation.findall('numerusform')]
                    self.assertEqual(len(forms), 1,
                                     'Chinese uses one plural form')
                else:
                    forms = [translation.text or '']
                for form in forms:
                    self.assertTrue(form.strip())
                    self.assertEqual(sorted(re.findall(r'%L?\d+|%n|%p', source)),
                                     sorted(re.findall(r'%L?\d+|%n|%p', form)))
                    self.assertEqual(re.findall(r'<[^>]+>', source),
                                     re.findall(r'<[^>]+>', form))
                    self.assertEqual(source.count('\n'), form.count('\n'))
                    # File dialog filter patterns must stay functional after translation.
                    self.assertEqual(re.findall(r'\*\.[a-z0-9]+|\(\*\.\*\)', source),
                                     re.findall(r'\*\.[a-z0-9]+|\(\*\.\*\)', form))


if __name__ == '__main__':
    unittest.main()
