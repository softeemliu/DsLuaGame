@echo off 

for %%f in (*.proto) do (
	@REM echo %%~nf
    protoc --proto_path=./ --descriptor_set_out=%%~nf.pb  %%f
	protoc-c --proto_path=./ --c_out=./ %%f
)

@REM protoc --proto_path=./ --descriptor_set_out=%%~nf.pb  person.proto

pause