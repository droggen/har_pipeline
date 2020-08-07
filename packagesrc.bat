@echo off
del /q har_pipeline-src.zip
zip -r har_pipeline.zip source\* -x *.svn*
zip -r har_pipeline.zip README.md
