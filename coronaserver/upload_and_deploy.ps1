
# Step 0: Login to Azure
az login

# Step 1: Define variables
$localFile = "D:\countrybit\x64\Release\revolutionservice.exe"
$storageAccount = "cvgtransferstorage"
$containerName = "xfer"
$blobName = "revolutionservice.exe"
$vmName = "coronaservers"
$resourceGroup = "coronaserver_group"
$destinationPath = "E:\revolution\database\revolutionservice.exe"

# Step 2: Upload the file to Azure Storage
az storage blob upload `
  --account-name $storageAccount `
  --container-name $containerName `
  --name $blobName `
  --file $localFile `
  --auth-mode login

# Step 3: Generate SAS token (valid for 1 hour)
$expiry = (Get-Date).AddHours(1).ToString("yyyy-MM-ddTHH:mm:ssZ")
$sasToken = az storage blob generate-sas `
  --account-name $storageAccount `
  --container-name $containerName `
  --name $blobName `
  --permissions r `
  --expiry $expiry `
  --auth-mode login `
  --output tsv

# Step 4: Construct full blob URL with SAS token
$blobUrl = "https://$storageAccount.blob.core.windows.net/$containerName/$blobName?$sasToken"

# Step 5: Use VM Custom Script Extension to download and move the file
$settings = @{
  fileUris = @($blobUrl)
  commandToExecute = "powershell -Command `"New-Item -ItemType Directory -Path 'E:\revolution\database' -Force; Copy-Item -Path .\revolutionservice.exe -Destination '$destinationPath' -Force`""
}

az vm extension set `
  --resource-group $resourceGroup `
  --vm-name $vmName `
  --name CustomScriptExtension `
  --publisher Microsoft.Compute `
  --settings ($settings | ConvertTo-Json -Compress)
