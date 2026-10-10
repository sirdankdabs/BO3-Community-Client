import importlib.util,struct,tempfile,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('client',Path(__file__).parents[1]/'app/client.py');client=importlib.util.module_from_spec(spec);spec.loader.exec_module(client)
class ProtocolTest(unittest.TestCase):
 def test_master(self):
  data=client.OOB+b'getserversResponse\\'+bytes([127,0,0,1])+struct.pack('!H',27017)+b'\\EOT'
  self.assertEqual(client.parse_master(data),['127.0.0.1:27017'])
  self.assertEqual(client.parse_master(data[:-7]),[])
 def test_endpoint(self):
  self.assertEqual(client.endpoint('127.0.0.1:27017'),('127.0.0.1',27017))
  for bad in ['127.0.0.1:0','127.0.0.1:65536','host;quit:1']:
   with self.assertRaises(ValueError):client.endpoint(bad)
 def test_json(self):
  with tempfile.TemporaryDirectory() as folder:
   p=Path(folder)/'request';client.atomic_json(p,{'name':'test'});self.assertEqual(client.json.loads(p.read_text()),{'name':'test'})
 def test_pe(self):
  with tempfile.TemporaryDirectory() as folder:
   p=Path(folder)/'game.exe';data=bytearray(256);data[:2]=b'MZ';struct.pack_into('<I',data,60,64);data[64:68]=b'PE\0\0';struct.pack_into('<I',data,152,0x6531394);p.write_bytes(data)
   self.assertEqual(client.executable_checksum(p),0x6531394)
if __name__=='__main__':unittest.main()
