# Preview plate coordinate regression

Open `preview-plate-coordinates.3mf`. It contains one 20 mm cube on each of three plates, using Bambu Lab H2D 0.4 nozzle, Generic PETG, and 0.20mm Standard.

1. Slice the three plates and switch to Preview.
2. Select a plate in the thumbnail list.
3. At the last layer, move the bottom slider left by one step.
4. Compare the highlighted G-code endpoint with the X/Y readout at the bottom.

The selected travel ends at X165.323 Y151.093 Z20.000. Plate 2 has scene origin (420, 0), and plate 3 has scene origin (0, -384). Before the fix, those offsets are included in the readout. The first plate already displays the expected coordinates.

Screenshots are original application captures. No physical printing is involved.

## Observed results

| Case | G-code X/Y | Before readout X/Y | Fixed readout X/Y |
|---|---|---|---|
| Plate 1 | 165.323 / 151.093 | 165.323 / 151.093 | 165.323 / 151.093 |
| Plate 2 | 165.323 / 151.093 | 585.323 / 151.093 | 165.323 / 151.093 |
| Plate 3 | 165.323 / 151.093 | 165.323 / -232.907 | 165.323 / 151.093 |

The exported plate-2 G-code was also opened independently in the fixed application: the readout matched the G-code. The two plate-2 exports have identical sequences of 7,993 non-comment commands; run `python3 verify_exports.py` after extracting the reproduction ZIP to verify this. Comments, including generated timestamps, are excluded.

Validated on macOS 15.7.7 (24G720), Apple Silicon, using the existing RelWithDebInfo build configuration. Windows and Linux were not tested. This is a GUI validation; no physical print was performed.
