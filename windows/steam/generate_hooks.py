"""Enable pinned inventory/slot spoof blocks dynamically; retain license checks."""
import re,sys
from pathlib import Path
text=Path(sys.argv[1]).read_text()
for name in ('hkLiveInventory_GetItemQuantity','hkLiveInventory_AreExtraSlotsPurchased'):
    start=text.index(name+'(');end=text.index('\n\t\t}',start)+len('\n\t\t}')
    block=text[start:end]
    assert block.count('#if SPOOF_UNLOCK')==1 and block.count('#endif')==1
    block=block.replace('#if SPOOF_UNLOCK','if (BO3Community_UnlockEnabled()) {').replace('#endif','}')
    text=text[:start]+block+text[end:]
Path(sys.argv[2]).write_text('extern bool BO3Community_UnlockEnabled();\n'+text)
